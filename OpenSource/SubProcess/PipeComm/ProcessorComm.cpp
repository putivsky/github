//
//  ProcessorComm.cpp
//  ProcessorComm
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
#include <atomic>

#if defined(__APPLE__) || defined(__linux__)
#include <unistd.h>
#include <getopt.h>
#include <spawn.h>
#include <signal.h>
#include <sys/wait.h>
#elif defined(_WIN32)
#include <windows.h>
#else
#error Unknown operation system
#endif

#include "ProcessorComm.h"
#include "TransportComm.h"
#include "PipeComm.h"
#include <iostream>
#include <thread>
#include <optional>

namespace SP {

class CommProcess {
protected:
    
    std::unique_ptr<CommTransport> m_CommTransport;
    std::unique_ptr<PipeComm> m_pipeComm;
    std::unique_ptr<std::thread> m_thread;
    DESC m_childDesc{INVALID_DESC};
#if defined(_WIN32)
    DWORD m_childThreadId{ 0 };
#endif
    void CommInitialization(CommCallback* callback) {
        m_CommTransport = CommTransportFactory::getCommTransport();
        m_pipeComm = std::make_unique<PipeComm>(*m_CommTransport, callback);
    }
    
    void CommCompletionLoop(CommEvent* completion) {
        if (completion) {
            m_thread = std::make_unique<std::thread>([this, completion]() {                m_CommTransport->StartCompletionLoop(
                                          std::bind(&PipeComm::OnEventCallback,
                                                    m_pipeComm.get(),
                                                    std::placeholders::_1,
                                                    std::placeholders::_2,
                                                    std::placeholders::_3,
                                                    std::placeholders::_4),
                                                    completion);
                
            });
        } else {
            m_CommTransport->StartCompletionLoop(
                                      std::bind(&PipeComm::OnEventCallback,
                                                m_pipeComm.get(),
                                                std::placeholders::_1,
                                                std::placeholders::_2,
                                                std::placeholders::_3,
                                                std::placeholders::_4),
                                                nullptr);
        }
    }

    void CommBreakCompletionLoop() {
        m_CommTransport->BreakCompletionLoop();
    }
    
    int CommSend(MsgBody&& msg) {
        if (!m_pipeComm) {
            return EFAULT;
        }
        
        m_pipeComm->Write(std::forward<MsgBody>(msg));
        return 0;
    }
    
    void CommStop() {
        if (m_CommTransport) {
             m_CommTransport->Close();
        }
        
        if (m_thread) {
            m_thread->join();
            m_thread.reset();
        }
        
        m_pipeComm.reset();
        m_CommTransport.reset();
    }
};

class ParentProcess : public CommProcess, public ParentProcessInterface {
    std::atomic<size_t> m_cidNext = 0;
    DESC m_childDesc{INVALID_DESC};
    
private:
    int Start(int argc, const char** argv, const std::string& childProcess,
              CommCallback* callback, CommEvent* completion) override {
        
        CommInitialization(callback);

#if defined(__APPLE__) || defined(__linux__)
        const auto pid = ::getpid();
#elif defined(_WIN32)
        const auto pid = ::GetCurrentProcessId();

#else
#error Unknown operation system
#endif

        int err;
        
        std::string tempDir;
        if ((err = CommTransportFactory::FindTempDirectory(tempDir))) {
            return err;
        }

        // generate cid as unique numbeer within the parent process
        const auto cid = (int)++m_cidNext;

        if ((err = m_CommTransport->Initiate())) {
            return err;
        }
        
        if ((err = m_CommTransport->Create(pid, cid))) {
            return err;
        }
        
        // open read end
        if ((err = m_CommTransport->OpenReadEnd(cid, pid, true))) {
            return err;
        }

        // start subprocess
#if defined(__APPLE__) || defined(__linux__)
        // make command line arguments
        std::vector<const char*> argvCompleted;
        std::string pidParam = "--pid=" + std::to_string(pid);
        std::string cidParam = "--cid=" + std::to_string(cid);
        std::copy(argv, argv + argc, std::back_inserter(argvCompleted));
        argvCompleted.push_back(childProcess.c_str()); // first parameter must be process name
        argvCompleted.push_back(pidParam.c_str());
        argvCompleted.push_back(cidParam.c_str());
        argvCompleted.push_back(nullptr);

        std::vector<const char*> envpCompleted;
        std::string tempEnv = "TEMP=" + tempDir;
        envpCompleted.push_back(tempEnv.c_str());
        envpCompleted.push_back(nullptr);
        // span suprocess adding to the command line arguments pid & cid
        // adding to the environment temporary directory.
        if ((err = ::posix_spawn(&m_childDesc, childProcess.c_str(),
                            nullptr,//const posix_spawn_file_actions_t *restrict file_actions,
                            nullptr,//const posix_spawnattr_t *restrict attrp,
                            (char* const*)argvCompleted.data(),
                            (char* const*)envpCompleted.data()))) {
            return err;
        }
        
        // Important - wait till the child process starts and sends ready message
        
#elif defined(_WIN32)
        std::string cmdLIne;
        std::copy((const char*)argv, (const char*)argv + argc, std::back_inserter(cmdLIne));
        cmdLIne += " --pid=" + std::to_string(pid);
        cmdLIne += " --cid=" + std::to_string(cid);
        std::vector<const char*> envpCompleted;
        std::string tempEnv = "TEMP=" + tempDir;
        envpCompleted.push_back(tempEnv.c_str());
        envpCompleted.push_back(nullptr);

        STARTUPINFOA startupInfo = { sizeof(startupInfo) };
        PROCESS_INFORMATION processInfo = { 0 };
        char dirBuffer[MAX_PATH];
        ::GetModuleFileNameA(nullptr, dirBuffer, sizeof(dirBuffer));
 
        std::string fullPath(dirBuffer);
        // strip module name
        auto lastDash = fullPath.find_last_of('\\');
        fullPath.resize(lastDash);
        fullPath += "\\";
        fullPath += childProcess;
        if (!::CreateProcessA(fullPath.c_str(), cmdLIne.data(),
            nullptr, nullptr, FALSE, NORMAL_PRIORITY_CLASS, nullptr,
            nullptr,
            &startupInfo, &processInfo)) {
            return ::GetLastError();
        }

        m_childThreadId = processInfo.dwThreadId;
        m_childDesc = processInfo.hProcess;
#else
#error Unknown operation system
#endif

        if ((err = m_CommTransport->OpenWriteEnd(pid, cid, true))) {
            return err;
        }

        if ((err = m_CommTransport->EnableRead())) {
            return err;
        }

        CommCompletionLoop(completion);

        return 0;
    }
    
    int Send(MsgBody&& msg) override {
        return CommSend(std::forward<MsgBody>(msg));
    }
    
    void Stop() override { 
       CommBreakCompletionLoop();

        // kill subprocess
#if defined(__APPLE__) || defined(__linux__)
        ::kill(m_childDesc, SIGTERM);
        int status;
        ::waitpid(m_childDesc, &status, WUNTRACED | WCONTINUED);
        close(m_childDesc);
#elif defined(_WIN32)
        if (!::PostThreadMessage(m_childThreadId, WM_QUIT, 0, 0)) {
            printf("Parent failed to send WN_QUIT for thread: %d\n", m_childThreadId);
        }
        ::WaitForSingleObject(m_childDesc, INFINITE);
        ::CloseHandle(m_childDesc);
#else
#error Unknown operation system
#endif
        CommStop();
    }
};

class ChildProcess : public CommProcess, public ChildProcessInterface {
private:
    int Start(int pid, int cid, const std::string& tmpDir,
              CommCallback* callback, CommEvent* completion) override {
        CommInitialization(callback);

        int err;
        // open read end
        if ((err = m_CommTransport->Initiate())) {
            return err;
        }

        // open read end
        if ((err = m_CommTransport->OpenReadEnd(pid, cid, false))) {
            return err;
        }

        // open write end
        if ((err = m_CommTransport->OpenWriteEnd(cid, pid, false))) {
            return err;
        }

        if ((err = m_CommTransport->EnableRead())) {
            return err;
        }

        CommCompletionLoop(completion);

        return 0;
    }
    
    int Send(MsgBody&& msg) override {
        return CommSend(std::forward<MsgBody>(msg));
    }
    
    void Stop() override {
        CommBreakCompletionLoop();
        CommStop();
    }
};

std::unique_ptr<ParentProcessInterface> ProcessAPIFactory::getParent() {
    return std::make_unique<ParentProcess>();
}
    
std::unique_ptr<ChildProcessInterface> ProcessAPIFactory::getChild() {
    return std::make_unique<ChildProcess>();
}

bool ProcessAPIFactory::parseCmdArguments(int argc, char* argv[], int* pid, int* cid) {
    std::optional<int> pidOpt, cidOpt;
  
#if defined(__APPLE__) || defined(__linux__)
    static struct option loptions[] = {
       {"pid", required_argument, 0,  'p' },
       {"cid", required_argument, 0,  'c' },
       {0,     0,                 0,   0 }
    };

    for (int i = 1; i < argc; ++i) {
        int oindex = 0;
        int res = getopt_long(argc, argv, "p:c:", loptions, &oindex);
        switch (res) {
            case 'p':
                pidOpt = atoi(optarg);
                break;
            case 'c':
                cidOpt = atoi(optarg);
                break;
        }
    }
    
#elif defined (_WIN32)
    static const char* loptions[] = {
        "--pid=", "--cid="
    };

    for (int i = 1; i < argc; ++i) {
        for (int l = 0; l < sizeof(loptions)/sizeof(*loptions); ++l) {
            std::string key(loptions[l]), keyValue(argv[i]);
            auto idx = keyValue.find(key);
            if (idx == 0) {
                switch (l) {
                case 0:
                    pidOpt = atoi(keyValue.substr(key.size()).c_str());
                    break;
                case 1:
                    cidOpt = atoi(keyValue.substr(key.size()).c_str());
                    break;
                }
            }
        }
    }
#else
#endif

    if (!pidOpt || !cidOpt || pidOpt.value() == 0 || cidOpt.value() == 0) {
        return false;
    }

    *pid = pidOpt.value();
    *cid = cidOpt.value();
    return true;
}


class TestParentProcess : public CommProcess, public ParentProcessInterface {
    std::atomic<size_t> m_cidNext{0};
    std::unique_ptr<std::thread> m_thread;
    std::unique_ptr<ChildProcessInterface> m_child;
    
    class TestChildCommCallback : public CommCallback {
        ChildProcessInterface* m_child{nullptr};
    public:
        void SetChild(ChildProcessInterface* child) {
            m_child = child;
        }
    private:
        void OnReadMsg(MsgBody&& msg) override {
            if (!msg.empty()) {
                printf("Child read msg, size=%zu, send half back\n", msg.size());
                msg.resize(msg.size() / 2);
                m_child->Send(std::forward<SP::MsgBody>(msg));
            } else {
                printf("Child read msg, size=%zu\n", msg.size());
            }
        }
        void OnWriteMsg() override {
            printf("Child write msg\n");
        }
        void OnError(int code) override {
            printf("Child error, code=%d\n", code);
        }
        void OnClose() override {
            printf("Child close\n");
        }
    };
    
    TestChildCommCallback m_callback;
protected:
    int Start(int, const char**, const std::string&,
              CommCallback* callback, CommEvent* completion) override {
        
        CommInitialization(callback);

#if defined(__APPLE__) || defined(__linux__)
        const auto pid = ::getpid();
#elif defined(_WIN32)
        const auto pid = GetCurrentProcessId();
#else
#error Unknown operation system
#endif

        int err;
        
        std::string tempDir;
        if ((err = CommTransportFactory::FindTempDirectory(tempDir))) {
            return err;
        }

        // generate cid as unique
        const auto cid = (int)++m_cidNext;

        if ((err = m_CommTransport->Initiate())) {
            return err;
        }

        if ((err = m_CommTransport->Create(pid, cid))) {
            return err;
        }
        
        // open read end
        if ((err = m_CommTransport->OpenReadEnd(cid, pid, true))) {
            return err;
        }

        // run child as a in-process library
        m_child = ProcessAPIFactory::getChild();
        m_callback.SetChild(m_child.get());
        
        CommEvent ev{false, false};
        if ((err = m_child->Start(pid, cid, tempDir, &m_callback, &ev))) {
            return err;
        }
        
        ev.Wait(~0);
        
        // open write end
        if ((err = m_CommTransport->OpenWriteEnd(pid, cid, true))) {
            return err;
        }

        if ((err = m_CommTransport->EnableRead())) {
            return err;
        }

        CommCompletionLoop(completion);

        return 0;
    }
    
    int Send(MsgBody&& msg) override {
        return CommSend(std::forward<MsgBody>(msg));
    }
    
    void Stop() override {
        CommBreakCompletionLoop();
        if (m_child) {
            m_child->Stop();
        }
        
        m_child.reset();
        CommStop();
    }
};

std::unique_ptr<ParentProcessInterface> ProcessTestFactory::getParent() {
    return std::make_unique<TestParentProcess>();
}
    
}
