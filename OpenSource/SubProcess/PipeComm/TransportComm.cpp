//
//  TransportComm.cpp
//  TransportComm
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

#include "TransportComm.h"
#include "ProcessorComm.h"
#include <assert.h>
#include <filesystem>
#include <string>
#include <thread>
#include <atomic>

namespace SP {

CommEvent::CommEvent(bool initalState, bool autoReset) :
m_signalState(initalState), m_autoReset(autoReset) {}

void CommEvent::Set() {
    std::unique_lock<std::mutex> lock(m_mutex);
    
    m_signalState = true;
    lock.unlock();
    m_autoReset ? m_condition.notify_one() : m_condition.notify_all();
}

void CommEvent::Reset() {
    std::unique_lock<std::mutex> lock(m_mutex);
    if (!m_signalState) {
        return;
    }
    
    m_signalState = false;
}

bool CommEvent::Wait(size_t timeoutMs) {
    std::unique_lock lock(m_mutex);
    bool res = true;
    if (timeoutMs == ~0) {
        m_condition.wait(lock, [this] { return m_signalState; });
    } else {
        res = m_condition.wait_for(lock,
                                   std::chrono::milliseconds(timeoutMs), [this] { return m_signalState; });
    }
    
    if (m_autoReset && res) {
        m_signalState = false;
    }
    
    return res;
}

}

#if defined(__APPLE__) || defined(__linux__)
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>

#if defined(__APPLE__)
#include <sys/event.h>
#else
#include <sys/epoll.h>
#include <sys/eventfd.h>
#endif

namespace SP {

class CommTransportImpl : public CommTransport {
    // OS specific
    DESC m_transport{INVALID_DESC};
    DESC m_readEnd{INVALID_DESC};
    DESC m_writeEnd{INVALID_DESC};
    
    static constexpr uint64_t kIoWriteKey = 0;
    static constexpr uint64_t kIoReadKey = 1;
    static constexpr uint64_t kIoBreakKey = 2;

#if defined(__APPLE__)
    struct kevent m_events[3] = {0};
#else
    struct epoll_event m_events[3] = {0};
#endif
    
    std::thread::id m_loopThreadId;
    
    // Common members
    // Pipe name convention:
    // server->client (pid->cid) i.e. s2c, server opens from write, client opens for read
    // client->server (cid->pid) i.e. c2s, client opens for write, server open for read
    std::string m_s2cName, m_c2sName;
    CommCallback* m_callback{nullptr};
    std::atomic<bool> m_active{false};
    CommEvent m_loopEvent{true, false};
        
    int MakePipeName(int pid, int cid, std::string& name) {
        std::string path;
        int err;
        if ((err = CommTransportFactory::FindTempDirectory(path))) {
            return err;
        }
        name = path + '/'+ std::to_string(pid) + '-' + std::to_string(cid);
        return 0;
    }

    void Release() {
        if (INVALID_DESC != m_writeEnd) {
            ::close(m_writeEnd);
            m_writeEnd = INVALID_DESC;
        }
        
        if (INVALID_DESC != m_readEnd) {
            ::close(m_readEnd);
            m_readEnd = INVALID_DESC;
        }
        
        if (INVALID_DESC != m_transport) {
            ::close(m_transport);
            m_transport = INVALID_DESC;
        }
        
        if (!m_s2cName.empty()) {
            ::unlink(m_s2cName.c_str());
            m_s2cName.clear();
        }
        if (!m_c2sName.empty()) {
            ::unlink(m_c2sName.c_str());
            m_c2sName.clear();
        }
    }

    int OpenEnd(int from, int to, bool reader) {
        std::string name;
        int err;
        if ((err = MakePipeName(from, to, name))) {
            return err;
        }
        
        DESC& end = reader ? m_readEnd : m_writeEnd;
        end = ::open(name.c_str(),
                     (reader ? O_RDONLY : O_WRONLY) | (reader ? O_NONBLOCK : 0) | O_CLOEXEC,
                     reader ? S_IRUSR : S_IWUSR);
        if (INVALID_DESC == end) {
            return errno;
        }
                        
        return 0;
    }

public:
    CommTransportImpl() = default;
    ~CommTransportImpl() override { Release(); }
    
    // Creates the communication transport
    int Initiate() override {
#if defined(__APPLE__)
        if (INVALID_DESC == (m_transport = ::kqueue())) {
#else
        if (INVALID_DESC == (m_transport = ::epoll_create(1))) {
#endif
            auto err = errno;
            Close();
            return err;
        }
        
        m_active = true;
        return 0;
    }

    // Create the asynchronous completion mechanism
    // for the write/read calls between
    // parent process (pid) and child subprocess (cid).
    // Opens the pair of the named pipes
    int Create(int pid, int cid) override {
        int err;
        if ((err = MakePipeName(pid, cid, m_s2cName))) {
            Close();
            return err;
        }

        ::unlink(m_s2cName.c_str());
        if (::mkfifo(m_s2cName.c_str(), S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH | S_IFIFO)) {
            err = errno;
            m_s2cName.clear();
            Close();
            return err;
        }
 
        if ((err = MakePipeName(cid, pid, m_c2sName))) {
            Close();
            return err;
        }

        ::unlink(m_c2sName.c_str());
        if (::mkfifo(m_c2sName.c_str(), S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH | S_IFIFO)) {
            err = errno;
            m_c2sName.clear();
            Close();
            return err;
        }
       
        return 0;
    }
    
    // Opens communication channel for writing
    // between parent process (pid) and child subprocess (cid)
    // providing the completion callback, which must be valid
    // till Close() call.
    int OpenWriteEnd(int from, int to, bool) override {
        return OpenEnd(from, to, false);
    }
    
    // Opens communication channel for reading
    // between parent process (pid) and child subprocess (cid)
    int OpenReadEnd(int from, int to, bool) override {
        return OpenEnd(from, to, true);
    }
    
    // Runs until Close() call,
    // caller must set m_active = false and call m_loopEvent.Reset() first
    void StartCompletionLoop(EventCallback&& callback, CommEvent* completion) override {
        m_loopThreadId = std::this_thread::get_id();
        m_loopEvent.Reset();
        
#if defined(__APPLE__)
        // Associate the event with the pipe write end descriptor
        EV_SET(&m_events[0], m_writeEnd, EVFILT_WRITE, EV_ADD | EV_DISABLE, 0, 0, nullptr);
        if (-1 == ::kevent(m_transport, &m_events[kIoWriteKey], 1, nullptr, 0, nullptr)) {
            printf("kevent failed, err: %d\n", errno);
        }
        // Associate the event with the pipe read end descriptor
        EV_SET(&m_events[1], m_readEnd, EVFILT_READ, EV_ADD, 0, 0, nullptr);
        if (-1 == ::kevent(m_transport, &m_events[kIoReadKey], 1, nullptr, 0, nullptr)) {
            printf("kevent failed, err: %d\n", errno);
        }
#else
        m_events[kIoWriteKey].events = EPOLLOUT;
        m_events[kIoWriteKey].data.fd = m_writeEnd;
        if (-1 == ::epoll_ctl(m_transport, EPOLL_CTL_ADD, m_writeEnd, &m_events[kIoWriteKey])) {
            printf("epoll_ctl failed, err: %d\n", errno);
        }
        m_events[kIoReadKey].events = EPOLLIN;
        m_events[kIoReadKey].data.fd = m_readEnd;
        if (-1 == ::epoll_ctl(m_transport, EPOLL_CTL_ADD, m_readEnd, &m_events[kIoReadKey])) {
            printf("epoll_ctl failed, err: %d\n", errno);
        }
        
        m_events[kIoBreakKey].events = EPOLLIN;
        m_events[kIoBreakKey].data.fd = ::eventfd(0, EFD_SEMAPHORE| EFD_NONBLOCK);
        if (-1 == ::epoll_ctl(m_transport, EPOLL_CTL_ADD, m_events[kIoBreakKey].data.fd, &m_events[kIoBreakKey])) {
            printf("epoll_ctl failed, err: %d\n", errno);
        }
#endif
 
        if (completion) {
            completion->Set();
        }
        
        while (m_active) {
#if defined(__APPLE__)
            struct kevent event = {0};
            auto res = ::kevent(m_transport, nullptr, 0, &event, 1, nullptr);
            if (-1 == res) {
                break;
            }

            DESC handle = DESC(event.ident);
#else
            // wait for incoming events from KQ
            struct epoll_event event = {0};
            int timeoutMs = -1; // wait indefinitely
            auto res = ::epoll_wait(m_transport, &event, 1, timeoutMs);
            if (-1 == res) {
                break;
            }

            DESC handle = DESC(event.data.fd);
#endif
            res = 0;
            EventType type = EventType::Unknown;
            
#if defined(__APPLE__)
            if (event.flags & EV_EOF) {
                type = EventType::Eof;
            } else if (event.flags & EV_ERROR) {
                type = EventType::Error;
                res = int(event.data);
            } else if (EVFILT_USER == event.filter) {
                break;
            } else if (EVFILT_WRITE == event.filter) {
                type = EventType::Write;
            } else if (EVFILT_READ == event.filter) {
                type = EventType::Read;
            } else {
                assert(false);
            }
#else
            if (event.events & EPOLLERR) {
                type = EventType::Error;
            } else if (event.events & EPOLLOUT) {
                type = EventType::Write;
            } else if (event.events & EPOLLIN) {
                if (event.data.fd == m_readEnd) {
                    type = EventType::Read;
                } else if (event.data.fd == m_events[kIoBreakKey].data.fd) {
                    break;
                }
            } else {
                assert(false);
            }
#endif
            if (!callback(handle, type, res, 0)) {
                break;
            }
        }
        
#if defined(__APPLE__)
        EV_SET(&m_events[0], m_writeEnd, EVFILT_WRITE, EV_DELETE, 0, 0, nullptr);
        if (-1 == ::kevent(m_transport, &m_events[kIoWriteKey], 1, nullptr, 0, nullptr)) {
            printf("kevent failed, err: %d\n", errno);
        }
        EV_SET(&m_events[1], m_readEnd, EVFILT_READ, EV_DELETE, 0, 0, nullptr);
        if (-1 == ::kevent(m_transport, &m_events[kIoReadKey], 1, nullptr, 0, nullptr)) {
            printf("kevent failed, err: %d\n", errno);
       }
        EV_SET(&m_events[2], 0, EVFILT_USER, EV_DELETE, 0, 0, nullptr);
        if (-1 == ::kevent(m_transport, &m_events[kIoBreakKey], 1, nullptr, 0, nullptr)) {
            printf("kevent failed, err: %d\n", errno);
        }
#else
        if (-1 == ::epoll_ctl(m_transport, EPOLL_CTL_DEL, m_readEnd, nullptr)) {
            printf("epoll_ctl failed, err: %d\n", errno);
        }
        if (-1 == ::epoll_ctl(m_transport, EPOLL_CTL_DEL, m_writeEnd, nullptr)) {
            printf("epoll_ctl failed, err: %d\n", errno);
        }
        if (-1 == ::epoll_ctl(m_transport, EPOLL_CTL_DEL, m_events[kIoBreakKey].data.fd, nullptr)) {
            printf("epoll_ctl failed, err: %d\n", errno);
        }
        
        ::close(m_events[kIoBreakKey].data.fd);
#endif
        m_loopEvent.Set();
    }
    
    void BreakCompletionLoop() override {
        const auto releaseThread = std::this_thread::get_id();
        if (m_loopThreadId == releaseThread) {
            assert(false); // Can't release from the callback.
            return;
        }

        bool expected = true;
        if (m_active.compare_exchange_strong(expected, false)) {
            // Trigger loop break;
            // Prepare the loop break event
#if defined (__APPLE__)
            EV_SET(&m_events[kIoBreakKey], 0, EVFILT_USER, EV_ADD, NOTE_TRIGGER, 0, nullptr);
            if (-1 == ::kevent(m_transport, &m_events[kIoBreakKey], 1, nullptr, 0, nullptr)) {
                printf("kevent failed, err: %d\n", errno);
            }
#else
            if (-1 == ::eventfd_write(m_events[kIoBreakKey].data.fd, 0)) {
                printf("eventfd_write failed, err: %d\n", errno);
            }
#endif
            
            m_loopEvent.Wait(~0);
        }
    }

    // Activates write to the transport.
    int EnableWrite() override {
        // activate write
#if defined (__APPLE__)
        EV_SET(&m_events[kIoWriteKey], m_writeEnd, EVFILT_WRITE, EV_ADD | EV_ENABLE, 0, 0, nullptr);
        if (-1 == ::kevent(m_transport, &m_events[kIoWriteKey], 1, nullptr, 0, nullptr)) {
            int err = errno;
            printf("kevent failed, err: %d\n", err);
            return err;
        }
#else
        m_events[kIoWriteKey].events = EPOLLOUT;
        m_events[kIoWriteKey].data.fd = m_writeEnd;
        if (-1 == ::epoll_ctl(m_transport, EPOLL_CTL_MOD, m_events[kIoWriteKey].data.fd, &m_events[kIoWriteKey])) {
            int err = errno;
            printf("epoll_ctl failed, err: %d\n", errno);
            return err;
        }
#endif
        return 0;
    }

    int EnableRead() override {
        return 0; // always enabled
    }

    int DisableWrite() override {
#if defined (__APPLE__)
        EV_SET(&m_events[kIoWriteKey], m_writeEnd, EVFILT_WRITE, EV_DISABLE, 0, 0, nullptr);
        if (-1 == ::kevent(m_transport, &m_events[kIoWriteKey], 1, nullptr, 0, nullptr)) {
            printf("kevent failed, err: %d\n", errno);
        }
#else
        m_events[kIoWriteKey].events = EPOLLOUT | EPOLLET;
        m_events[kIoWriteKey].data.fd = m_writeEnd;
        if (-1 == ::epoll_ctl(m_transport, EPOLL_CTL_MOD, m_events[kIoWriteKey].data.fd, &m_events[kIoWriteKey])) {
            printf("epoll_ctl failed, err: %d\n", errno);
        }
#endif
        return 0;
    }

    void Close() override {
        Release();
    }
};
}

#elif defined(_WIN32)

namespace SP {
class CommTransportImpl : public CommTransport {
    // OS specific
    DESC m_transport{ INVALID_DESC };
    DESC m_readEnd{ INVALID_DESC };
    DESC m_writeEnd{ INVALID_DESC };
    OVERLAPPED m_readOv = { 0 };
    OVERLAPPED m_writeOv = { 0 };
    static constexpr uint64_t kIoWriteKey = 1;
    static constexpr uint64_t kIoReadKey = 2;
    static constexpr uint64_t kIoEnableWriteKey = 3;
    static constexpr uint64_t kIoEnableReadKey = 4;
    static constexpr uint64_t kIoBreakKey = 5;


    std::thread::id m_loopThreadId;

    // Common members
    // Pipe name convention:
    // server->client (pid->cid) i.e. s2c, server opens from write, client opens for read
    // client->server (cid->pid) i.e. c2s, client opens for write, server open for read
    CommCallback* m_callback{ nullptr };
    std::atomic<bool> m_active = false;
    CommEvent m_loopEvent{ true, false };

    int MakePipeName(int pid, int cid, std::string& name) {
        name = "\\\\.\\pipe\\" + std::to_string(pid) + "-" + std::to_string(cid);
        return 0;
    }

    void Release() {
        if (INVALID_DESC != m_writeEnd) {
            ::CloseHandle(m_writeEnd);
            m_writeEnd = INVALID_DESC;
        }

        if (INVALID_DESC != m_readEnd) {
            ::CloseHandle(m_readEnd);
            m_readEnd = INVALID_DESC;
        }

        if (INVALID_DESC != m_transport) {
            ::CloseHandle(m_transport);
            m_transport = INVALID_DESC;
        }
    }

    int OpenEnd(int from, int to, bool reader, bool server) {
        // on server size use ConnectNamedPipe
        // on client side CreateFile
        int err;
        if (server) {
            if (!::ConnectNamedPipe(reader ? m_readEnd : m_writeEnd, reader ? &m_readOv : &m_writeOv)) {
                err = ::GetLastError();
                if (err != ERROR_IO_PENDING && err != ERROR_PIPE_LISTENING && (err != ERROR_PIPE_CONNECTED || reader)) {
                    return err;
                }
            }
        } else {
            std::string name;
            if ((err = MakePipeName(from, to, name))) {
                return err;
            }

            if (INVALID_DESC == ((reader ? m_readEnd : m_writeEnd) = ::CreateFileA(name.c_str(),
                reader ? GENERIC_READ : GENERIC_WRITE,
                reader ? FILE_SHARE_READ : FILE_SHARE_WRITE,
                nullptr,
                OPEN_EXISTING,
                FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED,
                nullptr))) {
                err = GetLastError();
                return err;
            }

            if (INVALID_DESC == ::CreateIoCompletionPort(
                reader ? m_readEnd : m_writeEnd, 
                m_transport, 
                reader ? kIoReadKey : kIoWriteKey, 0)) {
                err = ::GetLastError();
                return err;
            }
        }

        return 0;
    }

public:
    CommTransportImpl() = default;
    ~CommTransportImpl() override { Release(); }

    // Creates the communication transport
    int Initiate() override {
        if (INVALID_DESC == (m_transport = ::CreateIoCompletionPort(INVALID_HANDLE_VALUE, nullptr, 0, 0))) {
            Close();
            return errno;
        }

        m_active = true;
        return 0;
    }

    // Create the asynchronous completion mechanism
    // for the write/read calls between
    // parent process (pid) and child subprocess (cid).
    // Opens the pair of the named pipes
    int Create(int pid, int cid) override {
        int err;
        std::string s2cName;
        if ((err = MakePipeName(pid, cid, s2cName))) {
            Close();
            return err;
        }

        if (INVALID_DESC == (m_writeEnd = ::CreateNamedPipeA(s2cName.c_str(),
            PIPE_ACCESS_OUTBOUND | FILE_FLAG_OVERLAPPED | FILE_FLAG_FIRST_PIPE_INSTANCE,
            PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT,
            1,
            512, 512,
            0,
            nullptr))) {
            err = GetLastError();
            Close();
            return err;
        }

        if (INVALID_DESC == ::CreateIoCompletionPort(m_writeEnd, m_transport, kIoWriteKey, 0)) {
            Close();
            return errno;
        }

        std::string c2sName;
        if ((err = MakePipeName(cid, pid, c2sName))) {
            Close();
            return err;
        }

        if (INVALID_DESC == (m_readEnd = ::CreateNamedPipeA(c2sName.c_str(),
            PIPE_ACCESS_INBOUND | FILE_FLAG_OVERLAPPED | FILE_FLAG_FIRST_PIPE_INSTANCE,
            PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT,
            1,
            512, 512,
            0,
            nullptr))) {
            err = GetLastError();
            Close();
            return err;
        }

        if (INVALID_DESC == ::CreateIoCompletionPort(m_readEnd, m_transport, kIoReadKey, 0)) {
            Close();
            return errno;
        }

        return 0;
    }

    // Opens communication channel for writing
    // between parent process (pid) and child subprocess (cid)
    // providing the completion callback, which must be valid
    // till Close() call.
    int OpenWriteEnd(int from, int to, bool server) override {
        return OpenEnd(from, to, false, server);
    }

    // Opens communication channel for reading
    // between parent process (pid) and child subprocess (cid)
    int OpenReadEnd(int from, int to, bool server) override {
        return OpenEnd(from, to, true, server);
    }

    // Runs until Close() call,
    // caller must set m_active = false and call m_loopEvent.Reset() first
    void StartCompletionLoop(EventCallback&& callback, CommEvent* completion) override {
        m_loopThreadId = std::this_thread::get_id();
        m_loopEvent.Reset();
        if (completion) {
            completion->Set();
        }

        while (m_active) {
            DWORD numberOfBytesTransferred{ 0 };
            ULONG_PTR completionKey = { 0 };
            LPOVERLAPPED lpOverlapped = { nullptr };
            if (!::GetQueuedCompletionStatus(
                m_transport, &numberOfBytesTransferred, &completionKey, &lpOverlapped, INFINITE)) {
                printf("GetQueuedCompletionStatus failed, err: %d\n", ::GetLastError());
                break;
            }

            DESC handle = INVALID_DESC;
            EventType type = EventType::Unknown;

            int res = 0;
            if (completionKey == kIoWriteKey || completionKey == kIoEnableWriteKey) {
                handle = m_writeEnd;
                type = EventType::Write;
            }
            else if (completionKey == kIoReadKey || completionKey == kIoEnableReadKey) {
                handle = m_readEnd;
                type = EventType::Read;
             }
            else if (completionKey == kIoBreakKey) {
                // cancel read IO
                ::CancelIo(m_readEnd);
                break;
            }
            else {
                assert(false);
            }
 
            if (!callback(handle, type, res, numberOfBytesTransferred)) {
                printf("Callback failed, key: %d\n", completionKey);
                break;
            }
        }

        m_loopEvent.Set();
    }

    void BreakCompletionLoop() override {
         const auto releaseThread = std::this_thread::get_id();
         if (m_loopThreadId == releaseThread) {
             assert(false); // Can't release from the callback.
             return;
         }

         bool expected = true;
         if (m_active.compare_exchange_strong(expected, false)) {
             // Trigger loop break;
             // Prepare the loop break event
             if (!::PostQueuedCompletionStatus(m_transport, 0, kIoBreakKey, nullptr)) {
                 printf("PostQueuedCompletionStatus failed, err: %d\n", ::GetLastError());
             }

             m_loopEvent.Wait(~0);
         }
    }

    // Activates write to the transport.
    int EnableWrite() override {
        // activate write
        if (!::PostQueuedCompletionStatus(m_transport, 0, kIoEnableWriteKey, nullptr)) {
            int err = GetLastError();
            printf("PostQueuedCompletionStatus failed, err: %d\n", err);
            return err;
        }

        return 0;
    }

    int EnableRead() override {
        // activate read
        if (!::PostQueuedCompletionStatus(m_transport, 0, kIoEnableReadKey, nullptr)) {
            int err = GetLastError();
            printf("PostQueuedCompletionStatus failed, err: %d\n", err);
            return err;
        }
        
        return 0;
    }
    
    // Activates read from the transport.
    int DisableWrite() override {
        return 0;
    }

    void Close() override {
        Release();
    }
};
}
#else
#error Unknown operation system
#endif

namespace SP {

/* static */
std::unique_ptr<CommTransport> CommTransportFactory::getCommTransport() {
    return std::make_unique<CommTransportImpl>();
}

/* static */
int CommTransportFactory::FindTempDirectory(std::string& path) {
    std::error_code ec;
    auto dirPath = std::filesystem::temp_directory_path(ec);
    if (ec) {
        const int code = ec.value();
        printf("std::filesystem::temp_directory_path failed, code: %d", code);
        return code;
    }
        
    std::string fullPath = dirPath.string();

    // Remove the trailng "/" if any.
    const auto strCut = fullPath.find_last_not_of("/");
    path = fullPath.substr(0, strCut + 1);
    return 0;
}

}

