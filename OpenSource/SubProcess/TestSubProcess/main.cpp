//
//  main.cpp
//  TestSubProcess
//
//  Created by Yuri Putivsky on 9/17/24.
//

#include <thread>
#include "ProcessorComm.h"

class TestCommCallback : public SP::CommCallback {
    SP::ParentProcessInterface* m_parent{nullptr};
public:
    TestCommCallback(SP::ParentProcessInterface* parent) : m_parent(parent) {}
private:
    void OnReadMsg(SP::MsgBody&& msg) override {
        if (!msg.empty()) {
            printf("Parent read msg, size=%zu, send half back\n", msg.size());
            msg.resize(msg.size() / 2);
            m_parent->Send(std::forward<SP::MsgBody>(msg));
        } else {
            printf("Parent read empty msg\n");
        }
    }
    void OnWriteMsg() override {
        printf("Parent write msg\n");
    }
    void OnError(int code) override {
        printf("Parent error, code: %d\n", code);
    }
    void OnClose() override {
        printf("Parent close\n");
    }
};

using namespace std::chrono_literals;

int main(int argc, char *argv[]) {
    SP::CommEvent ev{false, false};
    auto parent = 
    SP::ProcessTestFactory::getParent();  // in-process child
    //SP::ProcessAPIFactory::getParent(); // out of process child
    
    TestCommCallback callback(parent.get());

    int err;
    printf("Parent client start\n");

#if defined(__APPLE__) || defined(__linux__)
    if ((err = parent->Start(0, nullptr, "TestChildProcess", &callback, &ev))) {
#elif defined(_WIN32)
    if ((err = parent->Start(0, nullptr, "TestChildProcess.exe", &callback, &ev))) {
#else
#error Unknown operation system
#endif
        printf("Parent start - got error code: %d\n", err);
        parent->Stop();
        return err;
    }
    
    ev.Wait(~0);
    
    // initiate send
    SP::MsgBody buffer;
    buffer.resize(64);
    if ((err = parent->Send(std::move(buffer)))) {
        printf("Parent send - got error code: %d\n", err);
    }

    buffer.resize(1024);
    if ((err = parent->Send(std::move(buffer)))) {
        printf("Parent send - got error code: %d\n", err);
    }

    buffer.resize(4096);
    if ((err = parent->Send(std::move(buffer)))) {
        printf("Parent send - got error code: %d\n", err);
    }

    std::this_thread::sleep_for(2s);
    parent->Stop();
    printf("Parent is done!\n");
    return 0;
}
