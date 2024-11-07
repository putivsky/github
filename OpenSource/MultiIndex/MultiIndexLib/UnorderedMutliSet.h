//
//  UnorderedMutliSet.h
//  MultiIndex
//
//  Created by Yuri Putivsky on 10/12/24.
//

#pragma once

#include <vector>


struct UnorderedMultiSetSettings {
    UnorderedMultiSetSettings(size_t hashSize, float loadFactor) :
    minBucketCount(hashSize), maxLoadFactor(loadFactor) {}
    size_t minBucketCount;
    float maxLoadFactor;
};

// Index keeps list<Key>::iterator(s), which are essentially pointers
// therfore index nodes should be small in size, ideally just packed arrays of iterators
// to reduce the memory usage overhead.
// [0][1][2]...[M] - buckets
// [0] -> [0][1][2]...[N] - array of iterators groupped by keys
template <size_t Capacity, typename Iter, typename Pred>
class UnorderedMultiSet {
public:
    // just pointers
    using iterator = Iter*;
    using const_iterator = const Iter*;
private:
    struct Bucket {
        Iter* m_head{nullptr};
        size_t m_capacity{0};
        size_t m_size{0};
    };
            
    using BucketTable = std::vector<Bucket>;
    // Finds the range of the same keys in bucket, returns [first, last) range or
    // [0, 0) if no keys found.
    template<typename I, typename K>
    std::pair<I, I> EqualKeys(const Bucket& bucket, const K& key) const noexcept;
    // rehash the table
    void Rehash(size_t count) noexcept;
    // clear table
    static void ClearTable(BucketTable& table);
    // inserts new key into bucket and keeps the same keys grouped (not sorted) within bucket.
    std::pair<iterator, bool> BucketInsert(Bucket& bucket, const Iter& key) noexcept;
    
    template<typename I, typename K>
    inline I Find(const K& key) const noexcept;
    
    const UnorderedMultiSetSettings m_settings;
    const Pred m_compare; // hasher & equal operators
    BucketTable m_table; // buckets container
    size_t m_totalItems{0}; // keeps track of total number of items.

    UnorderedMultiSet(const UnorderedMultiSet& src) noexcept = delete;
    UnorderedMultiSet(UnorderedMultiSet&&) noexcept = delete;
    
protected:
    UnorderedMultiSet(TupleParams<Pred>&& params) noexcept;
    ~UnorderedMultiSet() noexcept;
    
    const Pred& key_comp() const noexcept { return m_compare; }
    
    // insert
    std::pair<iterator, bool> insert(const Iter& key) noexcept;
    
    // erase
    size_t erase(Iter key) noexcept;
    
    // equal_range
    template<typename I, typename K>
    std::pair<I, I> equal_range(const K& key) const noexcept;

    // find the first item by the key.
    template<typename I, typename K>
    I find(const K& key) const noexcept;
    
    template<typename I>
    I end() const noexcept { return nullptr; }
    
    // clear
    void clear() noexcept;
    
    // traverse
    void traverse() const noexcept;
};

#include "UnorderedMutliSet.hpp"
