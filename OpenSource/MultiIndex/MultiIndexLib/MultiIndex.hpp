//
//  MultiIndex.hpp
//  MultiIndex
//
//  Created by Yuri Putivsky on 10/8/24.
//
#include <stddef.h>

template<LockPolicy L, uint32_t Capacity, typename T, typename... P>
template<typename I, typename... ARGS>
MultiIndexTable<L, Capacity, T, P...>::CommonIndex<I, ARGS...>::CommonIndex(ARGS&&... args) noexcept :
    I(std::forward<ARGS>(args)...) {
}

template<LockPolicy L, uint32_t Capacity, typename T, typename... P>
template<typename I, typename... ARGS>
MultiIndexTable<L, Capacity, T, P...>::CommonIndex<I, ARGS...>::~CommonIndex() noexcept {
}
    
template<LockPolicy L, uint32_t Capacity, typename T, typename... P>
template<typename I, typename... ARGS>
typename MultiIndexTable<L, Capacity, T, P...>::ItersContainer
MultiIndexTable<L, Capacity, T, P...>::CommonIndex<I, ARGS...>::FindIterators(const T& where) const noexcept {
    ItersContainer result;
    for (auto p = this->equal_range(where); p.first != p.second; ++p.first) {
        result.push_back(*p.first);
    }
    return result;
}

template<LockPolicy L, uint32_t Capacity, typename T, typename... P>
template<typename I, typename... ARGS>
void
MultiIndexTable<L, Capacity, T, P...>::CommonIndex<I, ARGS...>::Insert(bool noRehash, const Iter& itRef, const BitRef affected) noexcept {
    if (affected) {
        this->insert(noRehash, itRef);
    }
}

template<LockPolicy L, uint32_t Capacity, typename T, typename... P>
template<typename I, typename... ARGS>
void
MultiIndexTable<L, Capacity, T, P...>::CommonIndex<I, ARGS...>::Update(const Iter& itRef, const T& what, BitRef isAffected) noexcept {
    isAffected = 0;
    for (auto p = this->equal_range(*itRef); p.first != p.second; ++p.first) {
        if (*p.first != itRef) {
            continue;
        }
        
        if (!this->is_equal(*itRef, what)) {
            this->erase(*p.first);
            isAffected = 1;
        }
        return;
    }
}

template<LockPolicy L, uint32_t Capacity, typename T, typename... P>
template<typename I, typename... ARGS>
void
MultiIndexTable<L, Capacity, T, P...>::CommonIndex<I, ARGS...>::Delete(const Iter& itRef) noexcept {
    for (auto p = this->equal_range(*itRef); p.first != p.second; ++p.first) {
        if (*p.first != itRef) {
            continue;
        }
        
        this->erase(*p.first);
        return;
    }
}

template<LockPolicy L, uint32_t Capacity, typename T, typename... P>
template<typename I, typename... ARGS>
std::optional<T>
MultiIndexTable<L, Capacity, T, P...>::CommonIndex<I, ARGS...>::FindFirst(const T& what) const noexcept {
    std::optional<T> result;
    auto it = this->find(what);
    if (it != this->end()) {
        result = std::cref(**it); // copyable
    }
    
    return result;
}

template<LockPolicy L, uint32_t Capacity, typename T, typename... P>
template<typename I, typename... ARGS>
typename MultiIndexTable<L, Capacity, T, P...>::ObjectContainer
MultiIndexTable<L, Capacity, T, P...>::CommonIndex<I, ARGS...>::FindAll(const T& what) const noexcept {
    ObjectContainer result;
    FindBySelector([&result](const T& item) { result.push_back(item); }, what);
    return result;
}

template<LockPolicy L, uint32_t Capacity, typename T, typename... P>
template<typename I, typename... ARGS>
template<typename S>
void
MultiIndexTable<L, Capacity, T, P...>::CommonIndex<I, ARGS...>::FindBySelector(S&& selector, const T& what) const noexcept {
    for (auto p = this->equal_range(what); p.first != p.second; ++p.first) {
        selector(**p.first);
    }
}

template<LockPolicy L, uint32_t Capacity, typename T, typename... P>
template<typename I, typename... ARGS>
void
MultiIndexTable<L, Capacity, T, P...>::CommonIndex<I, ARGS...>::Clear() noexcept {
    this->clear();
}

template<LockPolicy L, uint32_t Capacity, typename T, typename... P>
template<typename I, typename... ARGS>
void
MultiIndexTable<L, Capacity, T, P...>::CommonIndex<I, ARGS...>::Traverse() const noexcept {
    this->traverse();
}

/////////////////////////////////////////////////////// MultiIndexTable
template<LockPolicy L, uint32_t Capacity, typename T, typename... P>
MultiIndexTable<L, Capacity, T, P...>::MultiIndexTable(size_t hashSize, float maxFactor, P&&... predicates) noexcept :
    m_IndexObjects(std::make_tuple(hashSize, maxFactor, std::forward<P>(predicates))...) {
    static_assert(Capacity > 0);
}

template<LockPolicy L, uint32_t Capacity, typename T, typename... P>
MultiIndexTable<L, Capacity, T, P...>::~MultiIndexTable() noexcept {
    std::apply([&](auto&... idx) { // for all indexes
        (idx.Clear(), ...);
    }, m_IndexObjects);
    
    m_objects.clear();
}

template<LockPolicy L, uint32_t Capacity, typename T, typename... P>
void MultiIndexTable<L, Capacity, T, P...>::Insert(T&& obj, bool noRehash) noexcept {
    std::bitset<sizeof...(P)> affectedIndices(1);
    // lock
    WriteLock<L> locker(m_mutex);
    auto iter = m_objects.insert(m_objects.end(), std::forward<T>(obj));
    std::apply([&](auto&... idx) { // for all indexes
        (idx.Insert(noRehash, iter, affectedIndices[0]), ...);
    }, m_IndexObjects);
}

// Update by index
template<LockPolicy L, uint32_t Capacity, typename T, typename... P>
template<size_t I>
bool MultiIndexTable<L, Capacity, T, P...>::Update(const T& where, T&& what) noexcept {
    // check the index existance
    static_assert(I < sizeof...(P), "Index is out of range");
    // find the index by a position
    auto& idx = std::get<I>(m_IndexObjects);
    // lock
    WriteLock<L> locker(m_mutex);
    auto iters = idx.FindIterators(where);
    for (auto& iter : iters) {
        size_t indexPos = 0;
        std::bitset<sizeof...(P)> affectediIndices;
        std::apply([&](auto&... idx) { // for all indexes
            (idx.Update(iter, what, affectediIndices[indexPos++]), ...);
        }, m_IndexObjects);
        
        if (iters.size() == 1) {
            *iter = std::forward<T>(what);
        } else {
            *iter = std::cref(what); // must be copyable
        }
        
        indexPos = 0;
        std::apply([&](auto&... idx) { // for all indexes
            (idx.Insert(true, iter, affectediIndices[indexPos++]), ...);
        }, m_IndexObjects);
    }
    
    return !iters.empty();
}


// Delete by index
template<LockPolicy L, uint32_t Capacity, typename T, typename... P>
template<size_t I>
size_t MultiIndexTable<L, Capacity, T, P...>::Delete(const T& where) noexcept {
    // check the index existance
    static_assert(I < sizeof...(P), "Index is out of range");
    // find the index by a position
    auto& idx = std::get<I>(m_IndexObjects);
    // lock
    WriteLock<L> locker(m_mutex);
    // Find all candidates for deletion
    auto iters = idx.FindIterators(where);
    
    for (auto& iter : iters) {
        std::apply([&iter](auto&... idx) { // for all indexes
            (idx.Delete(iter), ...);
        }, m_IndexObjects);
        
        m_objects.erase(iter);
    }
 
    return iters.size();
}

// Search by index
template<LockPolicy L, uint32_t Capacity, typename T, typename... P>
template<size_t I>
std::optional<T> MultiIndexTable<L, Capacity, T, P...>::FindFirst(const T& what) const noexcept {
    // check the index existance
    static_assert(I < sizeof...(P), "Index is out of range");
    // find the index by a position
    const auto& idx = std::get<I>(m_IndexObjects);
    // lock
    ReadLock<L> locker(m_mutex);
    return idx.FindFirst(what);
}

template<LockPolicy L, uint32_t Capacity, typename T, typename... P>
template<size_t I>
typename MultiIndexTable<L, Capacity, T, P...>::ObjectContainer MultiIndexTable<L, Capacity, T, P...>::FindAll(const T& what) const noexcept {
    // check the index existance
    static_assert(I < sizeof...(P), "Index is out of range");
    // find the index by a position
    const auto& idx = std::get<I>(m_IndexObjects);
    // lock
    ReadLock<L> locker(m_mutex);
    return idx.FindAll(what);
}

template<LockPolicy L, uint32_t Capacity, typename T, typename... P>
template<size_t I, typename S>
void MultiIndexTable<L, Capacity, T, P...>::FindBySelector(S&& selector, const T& what) const noexcept {
    // check the index existance
    static_assert(I < sizeof...(P), "Index is out of range");
    // find the index by a position
    const auto& idx = std::get<I>(m_IndexObjects);
    // lock
    ReadLock<L> locker(m_mutex);
    idx.FindBySelector(std::forward<S>(selector), what);
}

template<LockPolicy L, uint32_t Capacity, typename T, typename... P>
void MultiIndexTable<L, Capacity, T, P...>::Clear() noexcept {
    // lock
    WriteLock<L> locker(m_mutex);
    std::apply([&](auto&... idx) { // for all indexes
        (idx.Clear(), ...);
    }, m_IndexObjects);
}

template<LockPolicy L, uint32_t Capacity, typename T, typename... P>
template<size_t I>
void MultiIndexTable<L, Capacity, T, P...>::Traverse() const noexcept {
    // check the index existance
    static_assert(I < sizeof...(P), "Index is out of range");
    // find the index by a position
    const auto& idx = std::get<I>(m_IndexObjects);
    ReadLock<L> locker(m_mutex);
    idx.Traverse();
}
