//
//  PipeComm.cpp
//  PipeComm
//
//  Created by Yuri Putivsky on 9/15/24.
//

// Pipe architecture.

// Server (parent) can start multiple subprocess (children) at any time
// and open two ways communication channel.
// That implies to use FIFO (named pipes).

// On Linux SIGPIPE signal must be suppressed for handling the pipe closure
// in the asynchronous calls API.

/*
 https://www.man7.org/linux/man-pages/man7/fifo.7.html
A process can open a FIFO in nonblocking mode.  In this case,
opening for read-only succeeds even if no one has opened on the
write side yet and opening for write-only fails with ENXIO (no
such device or address) unless the other end has already been
opened.

Under Linux, opening a FIFO for read and write will succeed both
in blocking and nonblocking mode.  POSIX leaves this behavior
undefined.  This can be used to open a FIFO for writing while
there are no readers available.  A process that uses both ends of
the connection in order to communicate with itself should be very
careful to avoid deadlocks.
 */

// According to the above documentation
// the order of opening the read/write ends is strictly defined,
// otherwise deadlock can occurred.

// 1. Parent process (server0 opens the read end of the pipe (server->client)
// 2. starts a subprocess (client) and trying to open the read end of the pipe
// (client->server), such call will be blocked till the client opens
// the same pipe end for writing
// 3. A client opens the write end of the pipe (client->server)
// 4. A client opens the read end of the pipe (server->client)
// already opened by the parent process for writing.
// 5. Server gets unblocked by a client call (see 4.).

// On Windows there is a similar to FIFO API (Named Pipes).
// However the asynchronous API architecture is quite different.

// On Linux/Mac OS API (epoll/kqueue) waits till
// write/read functionality gets available:
// read - pipe is not empty
// write - pipe is not full
// On Windows write/read calls must be initiated first
// and wait for completion using Completion Port or other completion APIs.

// Both approaches can be unified under the hood with the same public API.

#if defined(__APPLE__)  || defined(__linux__)
#include <unistd.h>
#include <arpa/inet.h>
#elif defined(_WIN32)
#include <winsock2.h>
#pragma comment( lib, "ws2_32.lib" )
#else
#error Unknown operation system
#endif

#include "PipeComm.h"
#include <assert.h>

namespace SP {

PipeComm::PipeComm(CommTransport& api, CommCallback* callback) : m_api(api), m_callback(callback) {}

void PipeComm::Write(MsgBody&& msg) {
    // transfer body bytes
    CommMsg commMsg;
    commMsg.m_body = std::forward<MsgBody>(msg);
    // add message to the queue
    std::unique_lock<std::mutex> lock(m_writeLock);
    m_writeQueue.push_back(std::move(commMsg));
    // activate write API
    m_api.EnableWrite();
    lock.unlock();
}

bool PipeComm::OnEventCallback(DESC handle, EventType type, int errorCode, uint32_t processed) {
    switch (type) {
        case EventType::Error:
        case EventType::Eof:
            return false;
        case EventType::Write:
        {
#if defined(_WIN32)
            if (processed == 0 && m_writeMsg) {
                return true; // nothing to process
            }
#endif
            if (!m_writeMsg) {
                // first call
                std::unique_lock<std::mutex> lock(m_writeLock);

                if (m_writeQueue.empty()) {
                    // no messages in the queue - disable write
                    m_api.DisableWrite();
                    return true;
                }
                
                // get message from the queue.
                m_writeMsg = std::move(m_writeQueue.front());
                m_writeQueue.pop_front();
                lock.unlock();

                // assign message size in the network byte order.
                m_writeMsg.value().m_size = htonl((uint32_t)m_writeMsg.value().m_body.size());
            }

            auto& wMsg = m_writeMsg.value();
            wMsg.m_offset += processed;
            const uint32_t kSizeOfMessage = sizeof(wMsg.m_size);
            const uint32_t kBodySize = (uint32_t)wMsg.m_body.size();

            if (wMsg.m_offset < kSizeOfMessage) { // writing the message size
                if (!HandleWriteResult(handle, wMsg, 
                    &wMsg.m_size + wMsg.m_offset, 
                    kSizeOfMessage - wMsg.m_offset)) {
                    return false;
                }
            } else if (wMsg.m_offset < kSizeOfMessage + kBodySize) { // writing the body
                if (!HandleWriteResult(handle, wMsg,
                    wMsg.m_body.data() + (wMsg.m_offset - kSizeOfMessage),
                    kBodySize - (wMsg.m_offset - kSizeOfMessage))) {
                    return false;
                }
            } else {
                // report the written message
                m_callback->OnWriteMsg();
                // reset current message.
                m_writeMsg.reset();
            }
            
            return true;
        }
        break;
        case EventType::Read:
        {
#if defined(_WIN32)
            if (processed == 0 && m_readMsg) {
                return true; // nothing to process
            }
#endif
            if (!m_readMsg) {
                // new empty message
                CommMsg msg;
                m_readMsg = std::move(msg);
            }
            
            // continue read bytes
            auto& rMsg = m_readMsg.value();
            rMsg.m_offset += processed;
            const uint32_t kSizeOfMessage = sizeof(rMsg.m_size);

            if (rMsg.m_offset < kSizeOfMessage) { // reading header
                // reading size bytes in network order
                if (!HandleReadResult(handle, rMsg,
                    &rMsg.m_size + rMsg.m_offset,
                    kSizeOfMessage - rMsg.m_offset)) {
                    return false;
                }
            } else {  
                if (rMsg.m_offset == kSizeOfMessage) {
                    // get the length of message in the host byte order
                    auto length = ntohl(rMsg.m_size);
                    rMsg.m_body.resize(length); 
                }
                
                const uint32_t kBodySize = (uint32_t)rMsg.m_body.size();
                
                if (rMsg.m_offset < kSizeOfMessage + kBodySize) { // reading body
                    if (!HandleReadResult(handle, rMsg,
                        rMsg.m_body.data() + (rMsg.m_offset - kSizeOfMessage),
                        kBodySize - (rMsg.m_offset - kSizeOfMessage))) {
                        return false;
                    }
                } else {
                    // report the complete message
                    m_callback->OnReadMsg(std::move(rMsg.m_body));
                    m_readMsg.reset();
                    // request read bytes again
                    //printf("Enable read again\n");
                    m_api.EnableRead();
                }
            }
            return true;
        }
        break;
        default:
            assert(false);
            return false;
    }
}

#if defined(__APPLE__) || defined(__linux__)
bool PipeComm::HandleReadResult(DESC handle, CommMsg& msg, void* buffer, uint32_t size) {
    auto bytes = ::read(handle, buffer, size);
    if (-1 == bytes) {
        auto err = errno;
        if (err != EAGAIN) {
            m_callback->OnError(err);
            return false;
        } else {
            
        }
    } else if (0 == bytes) {
        m_callback->OnError(ENODATA);
        return false;
    }
    msg.m_offset += (uint32_t)bytes;
    return true;
}

bool PipeComm::HandleWriteResult(DESC handle, CommMsg& msg, const void* buffer, uint32_t size) {
    auto bytes = ::write(handle, buffer, size);
    if (-1 == bytes) {
        auto err = errno;
        if (err != EAGAIN) {
            m_callback->OnError(err);
            return false;
        }
    } else if (0 == bytes) {
        m_callback->OnError(ENODATA);
        return false;
    }
    msg.m_offset += (uint32_t)bytes;
    return true;
}

#elif defined(_WIN32)
bool PipeComm::HandleReadResult(DESC handle, CommMsg& msg, void* buffer, uint32_t size) {
    ::memset(&msg.m_overlapped, 0, sizeof(msg.m_overlapped));
    if (!::ReadFile(handle, buffer, size, nullptr, &msg.m_overlapped)) {
        int err = ::GetLastError();
        if (ERROR_IO_PENDING != err) {
            m_callback->OnError(err);
            return false;
        }
    }
 
    return true;
}

bool PipeComm::HandleWriteResult(DESC handle, CommMsg& msg, const void* buffer, uint32_t size) {
    ::memset(&msg.m_overlapped, 0, sizeof(msg.m_overlapped));
    if (!::WriteFile(handle, buffer, size, nullptr, &msg.m_overlapped)) {
        int err = ::GetLastError();
        if (ERROR_IO_PENDING != err) {
            m_callback->OnError(err);
            return false;
        }
    }
    return true;
}
#else
#error Unknown operation system
#endif

}

