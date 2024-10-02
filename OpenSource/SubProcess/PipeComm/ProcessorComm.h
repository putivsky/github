//
//  ProcessorComm.h
//  ProcessorComm
//
//  Created by Yuri Putivsky on 9/15/24.
//

#pragma once

#include "TransportComm.h"
#include <list>
#include <mutex>
#include <optional>

namespace SP {

// Completion callback.
// Never call ProcessInterface::Stop() method from inside any callback method.
class CommCallback {
public:
    virtual void OnReadMsg(MsgBody&& msg) = 0;
    virtual void OnWriteMsg() = 0;
    virtual void OnError(int code) = 0;
    virtual void OnClose() = 0;
};

class ParentProcessInterface {
public:
    virtual ~ParentProcessInterface() = default;
    // Caller can provide the optional parameter @completion
    // and wait till it signals that all required initialization has been done.
    // If @completion parameter is nullptr then the call is blocking.
    virtual int Start(int argc, const char** argv, // command line arguments for the child process "pid", and "cid" are reserved.
                      const std::string& childProcess,
                      CommCallback* callback, CommEvent* completion) = 0;
    virtual int Send(MsgBody&& msg) = 0;
    virtual void Stop() = 0;
};

class ChildProcessInterface {
public:
    virtual ~ChildProcessInterface() = default;
    // Caller can provide the optional parameter @completion
    // and wait till it signals that all required initialization has been done.
    // If @completion parameter is nullptr then the call is blocking.
    virtual int Start(int pid, int cid, const std::string& tmpDir,
                      CommCallback* callback, CommEvent* completion) = 0;
    virtual int Send(MsgBody&& msg) = 0;
    virtual void Stop() = 0;
};

class ProcessAPIFactory {
public:
    static std::unique_ptr<ParentProcessInterface> getParent();
    static std::unique_ptr<ChildProcessInterface> getChild();
    static bool parseCmdArguments(int argc, char* argv[], int* pid, int* cid);
};

class ProcessTestFactory {
public:
    static std::unique_ptr<ParentProcessInterface> getParent();
};

}
