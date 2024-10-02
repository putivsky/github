//
//  PipeComm.h
//  PipeComm
//
//  Created by Yuri Putivsky on 9/15/24.
//

#pragma once

#include "TransportComm.h"
#include "ProcessorComm.h"
#include <list>
#include <mutex>
#include <optional>

namespace SP {

// class handles pipe read/write operations
class PipeComm {
    struct CommMsg {
        uint32_t m_size{0}; // message size buffer
        uint32_t m_offset{0}; // current offset for read/write operation
        MsgBody m_body; // body bytes
#if defined(_WIN32)
        OVERLAPPED m_overlapped = { 0 };
#endif
    };

    CommTransport& m_api;
    CommCallback* m_callback{nullptr};
    
    // Output messages queue
    std::mutex m_writeLock;
    std::list<CommMsg> m_writeQueue;
    
    // current message for writing
    std::optional<CommMsg> m_writeMsg;
    // current message for reading
    std::optional<CommMsg> m_readMsg;

    bool HandleReadResult(DESC handle, CommMsg& msg, void* buffer, uint32_t size);
    bool HandleWriteResult(DESC handle, CommMsg& msg, const void* buffer, uint32_t size);
public:
    PipeComm(CommTransport& api, CommCallback* callback);
    void Write(MsgBody&& msg);
    bool OnEventCallback(DESC handle, EventType type, int errorCode, uint32_t processed);
};

}
