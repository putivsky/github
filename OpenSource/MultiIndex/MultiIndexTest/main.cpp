//
//  main.cpp
//  MultiIndexTest
//
//  Created by Yuri Putivsky on 10/8/24.
//

#include "MultiIndex.h"
#include <stdio.h>
#include <string>
#include <compare>

#if defined(_WIN32)
#include<windows.h>
#include <psapi.h>
#elif defined(__linux__)
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include <unistd.h>
#else
#include <sys/sysctl.h>
#include <mach/mach.h>
#include <mach/message.h>  // for mach_msg_type_number_t
#include <mach/kern_return.h>  // for kern_return_t
#include <mach/task_info.h>
#endif

unsigned long long membytes(void) {
#if defined(_WIN32)
    PROCESS_MEMORY_COUNTERS memCounter;
    BOOL result = ::GetProcessMemoryInfo(::GetCurrentProcess(),
                                       &memCounter,
                                       sizeof( memCounter ));
    if (result == FALSE) {
        printf("error %d\n", (int)::GetLastError());
        return 0;
    }
    
    return memCounter.WorkingSetSize;
#elif defined (__linux__)
    
    long rss = 0;
    FILE* fp = nullptr;
    if ((fp = fopen("/proc/self/statm", "r")) == nullptr) {
        return 0;
    }
    if (fscanf(fp, "%*s%ld", &rss) != 1) {
        fclose(fp);
        return 0;
    }
    fclose(fp);
    return (size_t)(rss * sysconf(_SC_PAGESIZE));
    
#else
  kern_return_t error;
  mach_msg_type_number_t outCount;
  mach_task_basic_info_data_t taskinfo;

  taskinfo.virtual_size = 0;
  outCount = MACH_TASK_BASIC_INFO_COUNT;
  error = task_info(mach_task_self(), MACH_TASK_BASIC_INFO, (task_info_t)&taskinfo, &outCount);
  if (error == KERN_SUCCESS) {
     return taskinfo.resident_size;
  } else {
    printf("error %d\n", (int)error);
    return 0;
  }
#endif
}

struct Object {
    int i;
    std::string s;
    
    inline size_t operator()() const { // hash operator
        return std::hash<int>{}(i) + std::hash<std::string>{}(s);
    }
    
    inline bool operator==(const Object& o) const { // equal operator
        return i == o.i && s == o.s;
    }
    
    inline bool operator<(const Object& o) const { // less operator
        return i != o.i ? i < o.i : s < o.s;
    }
};

struct IndexHashedOrderedPredicate : HashedOrderedTraits {
    inline size_t operator()(const Object& o) const noexcept {
        return o();
    }

    inline bool operator()(const Object& x, const Object& y) const noexcept {
        return x < y;
    }
};

struct IndexUnOrderedPredicate : UnOrderedTraits {
    inline size_t operator()(const Object& o) const noexcept {
        return o();
    }
    
    inline bool operator()(const Object& x, const Object& y) const noexcept {
        return x == y;
    }
};

struct IndexOrderedPredicate : OrderedTraits {
    inline bool operator()(const Object& x, const Object& y) const noexcept {
        return x < y;
    }
};

int main() {
    constexpr int kRounds = 1024*1024;
    constexpr int kBuckets = 32;
    
    auto iMem = membytes();
    printf("Initial mem: %llu\n", iMem);
    
    IndexUnOrderedPredicate ind1;
    IndexOrderedPredicate ind2;
    IndexHashedOrderedPredicate ind3;
    MultiIndexTable<LockPolicy::External, kBuckets, Object, IndexUnOrderedPredicate, IndexOrderedPredicate, IndexHashedOrderedPredicate>
    table(kRounds / kBuckets, kBuckets, std::move(ind1), std::move(ind2), std::move(ind3));

    Object o1 = {1, "1"}, o2 = {2, "2"};
    Object o1copy1(o1), o1copy2(o2);
    
    table.Insert(std::move(o1copy1));
    auto range = table.FindAll<0>(o2);
    table.Delete<0>(o2);
    table.Delete<2>(o1);
    range = table.FindAll<0>(o1);
    range = table.FindAll<2>(o2);

    

    auto startTime = std::chrono::high_resolution_clock::now();

    std::srand((unsigned int)std::time(nullptr));
    for (int i = kRounds; i >= 0; --i) {
        auto v = std::rand() % (kRounds/kBuckets);
        Object ocopy = {v, std::to_string(v)};
        table.Insert(std::move(ocopy), true);
    }
        
    auto endTime = std::chrono::high_resolution_clock::now();
    // Difference is calculated
    auto delta = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime);

    auto tMem = membytes();
    
    printf(
#if defined (__linux__)
    "Done with Terimber: %ld mem: %llu\n"
#else
    "Done with Terimber: %lld mem: %llu\n"
#endif
           , delta.count(), tMem - iMem);
    auto resRange1 = table.FindAll<0>(o1);
    auto resRange2 = table.FindAll<1>(o1);
    auto resRange3 = table.FindAll<2>(o1);
    auto resRange4 = table.FindAll<0>(o2);
    auto resRange5 = table.FindAll<1>(o2);
    auto resRange6 = table.FindAll<2>(o2);
    
    resRange1.clear();
    resRange2.clear();
    resRange3.clear();

    table.FindBySelector<0>([&resRange1](const Object& item) { resRange1.push_back(item); }, o1);
    table.FindBySelector<1>([&resRange2](const Object& item) { resRange2.push_back(item); }, o1);
    table.FindBySelector<2>([&resRange3](const Object& item) { resRange3.push_back(item); }, o1);

    
    table.Update<1>(o2, std::move(o1copy1));
    resRange1 = table.FindAll<0>(o1);
    resRange2 = table.FindAll<1>(o1);
    resRange3 = table.FindAll<2>(o1);
    resRange4 = table.FindAll<0>(o2);
    resRange5 = table.FindAll<1>(o2);
    resRange6 = table.FindAll<2>(o2);

    table.Update<0>(o1, std::move(o1copy2));
    resRange1 = table.FindAll<0>(o1);
    resRange2 = table.FindAll<1>(o1);
    resRange3 = table.FindAll<2>(o1);
    resRange4 = table.FindAll<0>(o2);
    resRange5 = table.FindAll<1>(o2);
    resRange6 = table.FindAll<2>(o2);

    auto res1 = table.FindFirst<0>(o1);
    auto res2 = table.FindFirst<1>(o2);
    auto res3 = table.FindFirst<2>(o2);
    resRange1 = table.FindAll<0>(o1);
    resRange2 = table.FindAll<1>(o1);
    resRange3 = table.FindAll<2>(o1);
    resRange4 = table.FindAll<0>(o2);
    resRange5 = table.FindAll<1>(o2);
    resRange6 = table.FindAll<2>(o2);

    table.Delete<1>(o1);
    table.Delete<0>(o2);
 
    res1 = table.FindFirst<0>(o1);
    res2 = table.FindFirst<1>(o2);
    res3 = table.FindFirst<2>(o2);
    resRange1 = table.FindAll<0>(o1);
    resRange2 = table.FindAll<1>(o2);
    resRange3 = table.FindAll<2>(o1);

    table.Clear();
}

