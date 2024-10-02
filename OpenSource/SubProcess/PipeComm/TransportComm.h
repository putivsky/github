//
//  TransportComm.h
//  TransportComm
//
//  Created by Yuri Putivsky on 9/15/24.
//

#pragma once

#include <functional>
#include <string>
#include <vector>
#include <memory>
#include <condition_variable>

#if defined(__APPLE__) || defined(__linux__)
#elif defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#error Unknown operation system
#endif

namespace SP {

class CommEvent {
public:
    CommEvent(bool initialState, bool autoReset);
    void Set();
    void Reset();
    bool Wait(size_t timeoutMs);
    
private:
    bool m_signalState;
    const bool m_autoReset;
    std::mutex m_mutex;
    std::condition_variable m_condition;
};

enum class EventType {
    Unknown = 0,
    Write,
    Read,
    Eof,
    Error
};

using MsgBody = std::vector<uint8_t>;

#if defined(__APPLE__) || defined(__linux__)

using DESC = int;
constexpr DESC INVALID_DESC = -1;

#elif defined(_WIN32)

using DESC = HANDLE;
const DESC INVALID_DESC = INVALID_HANDLE_VALUE;
#else
#error Unknown operation system
#endif

// When event got triggered API fires callback
// If it returns false, API stops processing events.
using EventCallback = std::function<bool(DESC handle, EventType type, int errorCode, uint32_t processed)>;

class CommTransport {
public:
    CommTransport() = default;
    virtual ~CommTransport() = default;
    
    // Creates the communication transport
    virtual int Initiate() = 0;

    // Create the asynchronous completion mechanism
    // for the write/read calls between
    // parent process (pid) and child subprocess (cid).
    // Opens the pair of the named pipes
    virtual int Create(int pid, int cid) = 0;
    
    // Opens a communication channel for writing
    // between parent process (pid) and child subprocess (cid)
    // providing the completion callback, which must be valid
    // till Close() call.
    virtual int OpenWriteEnd(int pid, int cid, bool server) = 0;
    
    // Opens a communication channel for reading
    // between parent process (pid) and child subprocess (cid)
    virtual int OpenReadEnd(int pid, int cid, bool server) = 0;
    
    // Runs until BreakCompletionLoop() call.
    virtual void StartCompletionLoop(EventCallback&& callback, CommEvent* completion) = 0;

    // Breaks the completion loop.
    virtual void BreakCompletionLoop() = 0;
    
    // Activates write to the transport.
    virtual int EnableWrite() = 0;

    // Activates read from the transport.
    virtual int EnableRead() = 0;

    // Activates read from the transport.
    virtual int DisableWrite() = 0;

    virtual void Close() = 0;
};

class CommTransportFactory {
public:
    static std::unique_ptr<CommTransport> getCommTransport();
    static int FindTempDirectory(std::string& path);
};

}
