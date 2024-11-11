//
//  MultiIndex.h
//  MultiIndex
//
//  Created by Yuri Putivsky on 10/8/24.
//
#pragma once

#include <algorithm>
#include <bitset>
#include <list>
#include <optional>
#include <set>
#include <shared_mutex>
#include <tuple>
#include <vector>

#if defined(_WIN32)
#elif defined(__linux__)
#include <stddef.h>
#else
#endif

template<typename Pred>
using TupleParams = std::tuple<size_t, float, Pred>;

#include "HashedOrderedMultiSet.h"
#include "OrderedMultiSet.h"
#include "UnOrderedMultiSet.h"

enum class LockPolicy {
    Internal = 0, // API takes care the proper read/write locking
    External // caller should properly orginize access to the API in multi-threaded environment.
};

template<LockPolicy>
class ReadLock {
public:
    ReadLock(std::shared_mutex&);
};

template<>
class ReadLock<LockPolicy::External> {
public:
    ReadLock(std::shared_mutex&) {}
};

template<>
class ReadLock<LockPolicy::Internal> {
    std::shared_mutex& m_mutex;
public:
    ReadLock<LockPolicy::Internal>(std::shared_mutex& mutex) : m_mutex(mutex) {
        m_mutex.lock_shared();
    }
    
    ~ReadLock() {
        m_mutex.unlock_shared();
    }
};

template<LockPolicy>
class WriteLock {
public:
    WriteLock(std::shared_mutex&);
};

template<>
class WriteLock<LockPolicy::External> {
public:
    WriteLock(std::shared_mutex&) {}
};

template<>
class WriteLock<LockPolicy::Internal> {
    std::shared_mutex& m_mutex;
public:
    WriteLock(std::shared_mutex& mutex) : m_mutex(mutex) {
        m_mutex.lock();
    }
    
    ~WriteLock() {
        m_mutex.unlock();
    }
};

struct HashedOrderedTraits {};
// Unordered (hashed) and ordered index predicate must be derived from HashedOrderedTraits
// and define two operators, i.e.
// hash operator: size_t operator()(const T& first) const;
// less operator: bool operator()(const T& first, const T& second) const;

struct UnOrderedTraits {};
// Unordered index predicate must be derived from UnOrderedTraits
// and define two operators, i.e.
// hash operator: size_t operator()(const T& first) const;
// equal operator: bool operator()(const T& first, const T& second) const;

struct OrderedTraits {};
// Ordered index predicate must be derived from OrderedTraits
// and define one operator, i.e.
// less operator: bool operator(const T& first, const T& second) const;

// class indexing T class objects by multiple predicates as indexes.
// @Capacity defines the size of buckets for ordered and unordered indexes.
template<LockPolicy L, uint32_t Capacity, typename T, typename... P>
class MultiIndexTable
{
    using ObjectContainer = std::list<T>;
    using Iter = typename ObjectContainer::iterator;
    using BitRef = typename std::bitset<sizeof...(P)>::reference;
    using ItersContainer = std::list<Iter>;

    template<typename I, typename... ARGS>
    class CommonIndex : public I {
        CommonIndex(const CommonIndex& src) noexcept = delete;
    public:
 
        CommonIndex(ARGS&&... args) noexcept;
        CommonIndex(CommonIndex&& src) noexcept;
        ~CommonIndex() noexcept;
        
        ItersContainer FindIterators(const T& where) const noexcept;

        void Insert(const Iter& itRef, const BitRef affected) noexcept;
        void Update(const Iter& itRef, const T& what, BitRef isAffected) noexcept;
        void Delete(const Iter& itRef) noexcept;
        std::optional<T> FindFirst(const T& what) const noexcept;
        ObjectContainer FindAll(const T& what) const noexcept;
        template<typename S>
        void FindBySelector(const S& selector, const T& what) const noexcept;
        
        void Clear();
        void Traverse();
    };

    // converts predicates types into Hashed/Unordered/Ordered indexes.
    template<typename Pred, bool ordered, bool hashed>
    struct IdxType {};

    template<typename Pred>
    struct IdxType<Pred, true, true> {
        using Type = CommonIndex<HashedOrderedMultiSet<Capacity, Iter, Pred>, TupleParams<Pred>>;
    };
    
    template<typename Pred>
    struct IdxType<Pred, true, false> {
        using Type = CommonIndex<OrderedMultiSet<Capacity, Iter, Pred>, TupleParams<Pred>>;
    };
    
    template<typename Pred>
    struct IdxType<Pred, false, true> {
        using Type = CommonIndex<UnOrderedMultiSet<Capacity, Iter, Pred>, TupleParams<Pred>>;
    };

    // auto detection of the predicate type
    template<typename Pred>
    struct IdxDetector {
    private:
        static_assert(std::is_base_of<UnOrderedTraits, Pred>::value ||
                      std::is_base_of<OrderedTraits, Pred>::value ||
                      std::is_base_of<HashedOrderedTraits, Pred>::value,
                      "Predicate class must be derived from either OrderedTraits or UnOrderedTraits or HashedOrderedTraits");
    public:
        using Type = typename IdxType<Pred,
            std::is_base_of<OrderedTraits, Pred>::value || std::is_base_of<HashedOrderedTraits, Pred>::value,
            std::is_base_of<HashedOrderedTraits, Pred>::value || std::is_base_of<UnOrderedTraits, Pred>::value>::Type;
    };

    ObjectContainer m_objects;
    std::tuple<typename IdxDetector<P>::Type...> m_IndexObjects;
    mutable std::shared_mutex m_mutex;
    
public:
    // Constructor
    // @hashSize defines the unordered indices hash table size
    MultiIndexTable(size_t hashSize, float maxFactor, P&& ...predicates) noexcept;
    ~MultiIndexTable() noexcept;
    // operations - insert, update, delete, search.
    
    // Insert the new object and update all indexes.
    void Insert(T&& obj) noexcept;
    // Update affected objects by index and update all indices
    template<size_t I>
    bool Update(const T& where, T&& what) noexcept;
    // Delete affected objects by index and update all indices
    template<size_t I>
    size_t Delete(const T& where) noexcept;
    
    // Search by index, finds the first object by index or not
    // that matches @what.
    template<size_t I>
    std::optional<T> FindFirst(const T& what) const noexcept;
    // Finds the set of objects that matches @what by index.
    template<size_t I>
    ObjectContainer FindAll(const T& what) const noexcept;
    // Finds with selector - must have operator()(const T& item);
    template<size_t I, typename S>
    void FindBySelector(const S& selector, const T& what) const noexcept;
    
    void Clear() noexcept;
    
    // DEBUG
    template<size_t I>
    void Traverse() noexcept;
};

#include "MultiIndex.hpp"

