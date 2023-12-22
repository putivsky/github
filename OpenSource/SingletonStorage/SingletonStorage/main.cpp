#include "SingletonStorageFactory.h"
#include <string>
#include <thread>
#include <chrono>
#include <iostream>

namespace UTest {
class A {
    std::string _s;
public:
    A(const std::string& s) : _s(s) {}
};

class B {
    std::weak_ptr<A> _a;
public:
    B(const std::string& s) {
        _a = SM::SingletonStorageFactory::create<A>(s);
    }
};

class C {
    std::weak_ptr<B> _b;
public:
    C(const std::string& s) {
        _b = SM::SingletonStorageFactory::create<B>(s);
    }
};

class RecursiveB;

class RecursiveA {
    std::weak_ptr<RecursiveB> _b;
public:
    RecursiveA(const std::string& s) {
        _b = SM::SingletonStorageFactory::create<RecursiveB>(std::string(s));
    }
};

class RecursiveB {
    std::weak_ptr<RecursiveA> _a;
public:
    RecursiveB(std::string&& s) {
        _a = SM::SingletonStorageFactory::create<RecursiveA>(s);
    }
};

template<typename T>
class TemplateClass {
    T _t;
public:
    TemplateClass(const T& t) : _t(t) {}
};

class MultiArgsClass {
    std::tuple<int, double, std::string, std::string> _t;
public:
    MultiArgsClass(int i, double d, const std::string& cstr, std::string&& rval) :
    _t(std::make_tuple(i, d, cstr, rval)) {
    }
};

}

void BaseFunctionalTest() {
    SM::SingletonStorageFactory::reset();
    std::string s("1");
    SM::SingletonStorageFactory::create<UTest::C>(s);
    // Double creation -> should call get instead.
    auto wpC = SM::SingletonStorageFactory::create<UTest::C>(s);
    assert(wpC.lock() != nullptr);
    
    auto wpMArgs = SM::SingletonStorageFactory::create<UTest::MultiArgsClass>(
        1, 2.2, "str", std::string("ref"));
    assert(wpMArgs.lock() != nullptr);
    
    try {
        // Recursive creation throws exception.
        std::string sR("R");
        SM::SingletonStorageFactory::create<UTest::RecursiveB>(std::move(sR));
    } catch (const std::exception& x) {
        std::cout << "Exception: " << x.what() << std::endl;
    }

    try {
        // Recursive creation throws exception.
        std::string sR("R");
        SM::SingletonStorageFactory::create<UTest::RecursiveA>(sR);
    } catch (const std::exception& x) {
        std::cout << "Exception: " << x.what() << std::endl;
    }

    SM::SingletonStorageFactory::destroy<UTest::B>();
    // Double destruction - should be ignored.
    SM::SingletonStorageFactory::destroy<UTest::B>();

    try {
        std::string s2("2");
        // Creation after deletion is not allowed.
        SM::SingletonStorageFactory::create<UTest::B>(s2);
    } catch (const std::exception& x) {
        std::cout << "Exception: " << x.what() << std::endl;
    }

    // Double creation -> should call get instead.
    auto wpA = SM::SingletonStorageFactory::create<UTest::A>(s);
    assert(wpA.lock() != nullptr);

    auto wpA2 = SM::SingletonStorageFactory::get<UTest::A>();
    assert(wpA2.lock() != nullptr);

    auto wpB = SM::SingletonStorageFactory::get<UTest::B>();
    assert(wpB.lock() == nullptr);

    SM::SingletonStorageFactory::create<UTest::TemplateClass<int>>(5);
    SM::SingletonStorageFactory::create<UTest::TemplateClass<std::string>>(s);
    // Clear storage.
    SM::SingletonStorageFactory::clear();
    
    try {
        // Creation after deletion is not allowed.
        SM::SingletonStorageFactory::create<UTest::TemplateClass<int>>(2);
    } catch (const std::exception& x) {
        std::cout << "Exception: " << x.what() << std::endl;
    }

    try {
        // Creation after deletion is not allowed.
        auto wpMArgs = SM::SingletonStorageFactory::create<UTest::MultiArgsClass>(
            1, 2.2, "str", std::string("ref"));
    } catch (const std::exception& x) {
        std::cout << "Exception: " << x.what() << std::endl;
    }

    SM::SingletonStorageFactory::reset();
    // Storage is set into the initial state.
    // Now any type can be re-created.
    SM::SingletonStorageFactory::create<UTest::B>(s);
}

void MultiTreadedTest() {
    std::atomic<bool> active(false);
    std::atomic<bool> stopped(false);
    std::atomic<bool> cleared(false);
    std::atomic<int> rounds(0);
    
    std::list<std::thread> threads;
    for (int i = 0; i < 16; ++i) {
        threads.emplace_back([&active, &stopped, &cleared, &rounds, i]() {
            while (!active) {
                std::this_thread::yield();
            }
            
            int count = 10;
            while (!stopped) {
                ++rounds;
                std::string s("1");
                try {
                    if (!cleared) {
                        SM::SingletonStorageFactory::create<UTest::C>(s);
                        // Double creation -> should call get instead.
                        SM::SingletonStorageFactory::create<UTest::C>(s);
                    }
                    
                    if (!cleared) {
                        SM::SingletonStorageFactory::create<UTest::A>(s);
                    }

                    if (!cleared) {
                        SM::SingletonStorageFactory::create<UTest::B>(std::move(s));
                    }
                } catch (const std::exception& x) {
                    std::cout << "Exception: " << x.what() << std::endl;
                }
                
                
                SM::SingletonStorageFactory::destroy<UTest::B>();
                // Double destruction - should be ignored.
                SM::SingletonStorageFactory::destroy<UTest::B>();
                
                try {
                    std::string s2("2");
                    // Creation after deletion is not allowed.
                    if (!cleared) {
                        SM::SingletonStorageFactory::create<UTest::B>(s2);
                    }
                } catch (const std::exception& x) {
                    std::cout << "Exception: " << x.what() << std::endl;
                }
                
                // Double creation -> should call get instead.
                try {
                    if (!cleared) {
                        auto wpA = SM::SingletonStorageFactory::create<UTest::A>(s);
                        assert(wpA.lock() != nullptr);
                    }
                } catch (const std::exception& x) {
                    std::cout << "Exception: " << x.what() << std::endl;
                }

                SM::SingletonStorageFactory::get<UTest::A>();
                SM::SingletonStorageFactory::get<UTest::B>();
                
                try {
                    if (!cleared) {
                        SM::SingletonStorageFactory::create<UTest::TemplateClass<int>>(5);
                    }
                    if (!cleared) {
                        SM::SingletonStorageFactory::create<UTest::TemplateClass<std::string>>(s);
                    }
                } catch (const std::exception& x) {
                    std::cout << "Exception: " << x.what() << std::endl;
                }
                
                if (i % count == 0) {
                    if (i % 2 == 0) {
                        cleared = true;
                        SM::SingletonStorageFactory::clear();
                    } else {
                        SM::SingletonStorageFactory::reset();
                        cleared = false;
                        try {
                            // Recursive creation throws exception.
                            std::string sR("R");
                            SM::SingletonStorageFactory::create<UTest::RecursiveB>(std::move(sR));
                        } catch (const std::exception& x) {
                            std::cout << "Exception: " << x.what() << std::endl;
                        }
                        
                        try {
                            // Recursive creation throws exception.
                            std::string sR("R");
                            SM::SingletonStorageFactory::create<UTest::RecursiveA>(sR);
                        } catch (const std::exception& x) {
                            std::cout << "Exception: " << x.what() << std::endl;
                        }
                    }
                }
            }
        });
    }
    active = true;
    std::this_thread::sleep_for(std::chrono::duration<double, std::milli>(500));
    stopped = true;
    for (auto& t : threads) {
        t.join();
    }
    
    std::cout << "Threads ran " << rounds << " times\n";

    threads.clear();
}

int main(int argc, char** args) {
    
    BaseFunctionalTest();
    MultiTreadedTest();
    return 0;
}
