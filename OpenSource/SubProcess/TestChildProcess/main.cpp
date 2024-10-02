//
//  main.cpp
//  TestChildProcess
//
//  Created by Yuri Putivsky on 9/18/24.
//

#include <thread>
#include <cstdlib>
#include "ProcessorComm.h"

#if defined(__APPLE__) || defined(__linux__)
#include <signal.h>
static SP::CommEvent s_event{ false, false };
static void sighandler(int signum) {
    s_event.Set();
}
#elif defined(_WIN32)
#else
#error Unknown operation system
#endif

static std::unique_ptr<SP::ChildProcessInterface> s_child;

class TestCommCallback : public SP::CommCallback {
    SP::ChildProcessInterface* m_child{nullptr};
private:
    void OnReadMsg(SP::MsgBody&& msg) override {
        if (!msg.empty()) {
            printf("Child read msg, size=%zu, send half back\n", msg.size());
            msg.resize(msg.size() / 2);
            m_child->Send(std::forward<SP::MsgBody>(msg));
        } else {
            printf("Child read empty msg\n");
        }
    }
    void OnWriteMsg() override {
        printf("Child write msg\n");
    }
    void OnError(int code) override {
        printf("Child error, code: %d\n", code);
    }
    void OnClose() override {
        printf("Child close\n");
    }
public:
    TestCommCallback(SP::ChildProcessInterface* child) : m_child(child) {}
};

using namespace std::chrono_literals;

int main(int argc, char *argv[]) {
#if defined(__APPLE__) || defined(__linux__)
    struct sigaction sa;
    sa.sa_handler =  sighandler;
    sa.sa_flags = SA_RESTART;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGTERM, &sa, nullptr);
#elif defined(_WIN32)
#else
#error Unknown operation system
#endif

    int pid, cid;
    if (!SP::ProcessAPIFactory::parseCmdArguments(argc, argv, &pid, &cid)) {
        printf("Child can't parse arguments\n");
        return -1;
    }
        
    std::string tmpDir;
#if defined(__APPLE__) || defined(__linux__)
    auto tmpDirEnv = std::getenv("TEMP");
    if (!tmpDirEnv) {
        printf("No environment variable TEMP\n");
        return -1;
    }
    tmpDir = tmpDirEnv;
#endif


    s_child = SP::ProcessAPIFactory::getChild();
    
    TestCommCallback callback(s_child.get());
    // non-blocking call
    SP::CommEvent ev{false, false};
    printf("Child standalone Start\n");
    s_child->Start(pid, cid, tmpDir, &callback, &ev);
    ev.Wait(~0);

#if defined(__APPLE__) || defined(__linux__)
    s_event.Wait(~0);
#elif defined(_WIN32)
    int res;
    MSG msg;
    while ((res = GetMessage(&msg, nullptr, 0, 0)) != 0) {
        if (-1 == res) {
            printf("GetMessage failed, err: %d\n", GetLastError());
            break;
        }
    }
#else
#error Unknown operation system
#endif
    
    s_child->Stop();
    printf("Child is done!\n");
    return 0;
}
