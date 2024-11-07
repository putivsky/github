//
//  MultiIndex.hpp
//  MultiIndex
//
//  Created by Yuri Putivsky on 10/8/24.
//
#include <stddef.h>

template<LockPolicy L, size_t Capacity, typename T, typename... P>
template<typename I, typename... ARGS>
MultiIndexTable<L, Capacity, T, P...>::CommonIndex<I, ARGS...>::CommonIndex(ARGS&&... args) noexcept :
    I(std::forward<ARGS>(args)...) {
}

template<LockPolicy L, size_t Capacity, typename T, typename... P>
template<typename I, typename... ARGS>
MultiIndexTable<L, Capacity, T, P...>::CommonIndex<I, ARGS...>::CommonIndex(CommonIndex&& src) noexcept :
    I(static_cast<I&&>(src)) {
}

template<LockPolicy L, size_t Capacity, typename T, typename... P>
template<typename I, typename... ARGS>
MultiIndexTable<L, Capacity, T, P...>::CommonIndex<I, ARGS...>::~CommonIndex() noexcept {
}
    
template<LockPolicy L, size_t Capacity, typename T, typename... P>
template<typename I, typename... ARGS>
typename MultiIndexTable<L, Capacity, T, P...>::ItersContainer
MultiIndexTable<L, Capacity, T, P...>::CommonIndex<I, ARGS...>::FindIterators(const T& where) const noexcept {
    ItersContainer result;
    auto p = this->template equal_range<typename I::const_iterator>(where);
    while (p.first != p.second) {
        result.push_back(*p.first);
        ++p.first;
    }
    return result;
}

template<LockPolicy L, size_t Capacity, typename T, typename... P>
template<typename I, typename... ARGS>
void
MultiIndexTable<L, Capacity, T, P...>::CommonIndex<I, ARGS...>::Insert(const Iter& itRef, const BitRef affected) noexcept {
    if (affected) {
        this->insert(itRef);
    }
}

template<LockPolicy L, size_t Capacity, typename T, typename... P>
template<typename I, typename... ARGS>
void
MultiIndexTable<L, Capacity, T, P...>::CommonIndex<I, ARGS...>::Update(const Iter& itRef, const T& what, BitRef affected) noexcept {
    auto p = this->template equal_range<typename I::iterator>(*itRef);
    while (p.first != p.second) {
        if (*p.first == itRef) {
            if (this->key_comp()(*itRef, what) || this->key_comp()(what, *itRef)) {
                this->erase(*p.first);
                affected = 1;
            } else {
                affected = 0;
            }
            
            return;
        }
        
        ++p.first;
    }
    
    affected = 0;
}

template<LockPolicy L, size_t Capacity, typename T, typename... P>
template<typename I, typename... ARGS>
void
MultiIndexTable<L, Capacity, T, P...>::CommonIndex<I, ARGS...>::Delete(const Iter& itRef) noexcept {
    auto p = this->template equal_range<typename I::iterator>(*itRef);
    while (p.first != p.second) {
        if (*p.first == itRef) {
            this->erase(*p.first);
            return;
        }
        
        ++p.first;
    }
}

template<LockPolicy L, size_t Capacity, typename T, typename... P>
template<typename I, typename... ARGS>
std::optional<T>
MultiIndexTable<L, Capacity, T, P...>::CommonIndex<I, ARGS...>::FindFirst(const T& what) const noexcept {
    std::optional<T> result;
    auto it = this->template find<typename I::const_iterator>(what);
    if (it != this->template end<typename I::const_iterator>()) {
        result = std::cref(**it); // copyable
    }
    
    return result;
}

template<LockPolicy L, size_t Capacity, typename T, typename... P>
template<typename I, typename... ARGS>
typename MultiIndexTable<L, Capacity, T, P...>::ObjectContainer
MultiIndexTable<L, Capacity, T, P...>::CommonIndex<I, ARGS...>::FindAll(const T& what) const noexcept {
    ObjectContainer result;
    FindBySelector([&result](const T& item) {
        result.push_back(item);
    }, what);
    return result;
}

template<LockPolicy L, size_t Capacity, typename T, typename... P>
template<typename I, typename... ARGS>
template<typename S>
void
MultiIndexTable<L, Capacity, T, P...>::CommonIndex<I, ARGS...>::FindBySelector(const S& selector, const T& what) const noexcept {
    auto p = this->template equal_range<typename I::const_iterator>(what);
    
    while (p.first != p.second) {
        selector(**p.first);
        ++p.first;
    }
}

template<LockPolicy L, size_t Capacity, typename T, typename... P>
template<typename I, typename... ARGS>
void
MultiIndexTable<L, Capacity, T, P...>::CommonIndex<I, ARGS...>::Clear() {
    this->clear();
}

template<LockPolicy L, size_t Capacity, typename T, typename... P>
template<typename I, typename... ARGS>
void
MultiIndexTable<L, Capacity, T, P...>::CommonIndex<I, ARGS...>::Traverse() {
    this->traverse();
}
///////////////////////////////////////////////////////
template<LockPolicy L, size_t Capacity, typename T, typename... P>
MultiIndexTable<L, Capacity, T, P...>::MultiIndexTable(size_t hashSize, float maxFactor, P&&... predicates) noexcept :
    m_IndexObjects(std::make_tuple(hashSize, maxFactor, std::forward<P>(predicates))...) {
    static_assert(Capacity > 0);
}

template<LockPolicy L, size_t Capacity, typename T, typename... P>
MultiIndexTable<L, Capacity, T, P...>::~MultiIndexTable() noexcept {
    std::apply([&](auto&... idx) { // for all indexes
        (idx.Clear(), ...);
    }, m_IndexObjects);
    
    m_objects.clear();
}

template<LockPolicy L, size_t Capacity, typename T, typename... P>
void MultiIndexTable<L, Capacity, T, P...>::Insert(T&& obj) noexcept {
    // lock
    WriteLock<L> locker(m_mutex);
    auto iter = m_objects.insert(m_objects.end(), std::forward<T>(obj));
    std::bitset<sizeof...(P)> affectedIndices;
    affectedIndices.set();
    std::apply([&](auto&... idx) { // for all indexes
        (idx.Insert(iter, affectedIndices[0]), ...);
    }, m_IndexObjects);
}

// Update by index
template<LockPolicy L, size_t Capacity, typename T, typename... P>
template<size_t I>
bool MultiIndexTable<L, Capacity, T, P...>::Update(const T& where, T&& what) noexcept {
    // check the index existance
    static_assert(I < sizeof...(P), "Index is out of range");
    
    // find value by index
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
            (idx.Insert(iter, affectediIndices[indexPos++]), ...);
        }, m_IndexObjects);
    }
    
    return !iters.empty();
}


// Delete by index
template<LockPolicy L, size_t Capacity, typename T, typename... P>
template<size_t I>
size_t MultiIndexTable<L, Capacity, T, P...>::Delete(const T& where) noexcept {
    static_assert(I < sizeof...(P), "Index is out of range");
        
    // find value by index
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
template<LockPolicy L, size_t Capacity, typename T, typename... P>
template<size_t I>
std::optional<T> MultiIndexTable<L, Capacity, T, P...>::FindFirst(const T& what) const noexcept {
    static_assert(I < sizeof...(P), "Index is out of range");
    // find value by index
    auto& idx = std::get<I>(m_IndexObjects);
    // lock
    ReadLock<L> locker(m_mutex);
    return idx.FindFirst(what);
}

template<LockPolicy L, size_t Capacity, typename T, typename... P>
template<size_t I>
typename MultiIndexTable<L, Capacity, T, P...>::ObjectContainer MultiIndexTable<L, Capacity, T, P...>::FindAll(const T& what) const noexcept {
    static constexpr size_t idxSize = sizeof...(P);
    static_assert(I < idxSize, "Index is out of range");
    // find value by index
    auto& idx = std::get<I>(m_IndexObjects);
    // lock
    ReadLock<L> locker(m_mutex);
    return idx.FindAll(what);
}

template<LockPolicy L, size_t Capacity, typename T, typename... P>
template<size_t I, typename S>
void MultiIndexTable<L, Capacity, T, P...>::FindBySelector(const S& selector, const T& what) const noexcept {
    static_assert(I < sizeof...(P), "Index is out of range");
    // find value by index
    auto& idx = std::get<I>(m_IndexObjects);
    // lock
    ReadLock<L> locker(m_mutex);
    return idx.FindAll(what);
}

template<LockPolicy L, size_t Capacity, typename T, typename... P>
void MultiIndexTable<L, Capacity, T, P...>::Clear() noexcept {
    // lock
    WriteLock<L> locker(m_mutex);
    std::apply([&](auto&... idx) { // for all indexes
        (idx.Clear(), ...);
    }, m_IndexObjects);
}

template<LockPolicy L, size_t Capacity, typename T, typename... P>
template<size_t I>
void MultiIndexTable<L, Capacity, T, P...>::Traverse() noexcept {
    // find value by index
    auto& idx = std::get<I>(m_IndexObjects);
    idx.Traverse();
}
