//
//  HashedMultiSet.h
//  MultiIndex
//
//  Created by Yuri Putivsky on 10/12/24.
//

#pragma once

#include <vector>


struct HashedMultiSetSettings {
    HashedMultiSetSettings(size_t hashSize, float loadFactor) :
    minBucketCount(hashSize), maxLoadFactor(loadFactor) {}
    size_t minBucketCount;
    float maxLoadFactor;
};

// Index keeps list<Key>::iterator(s), which are essentially pointers
// therefore index nodes should be small in size, ideally just packed arrays of iterators
// to reduce the memory usage overhead.
// [0][1][2]...[M] - buckets
// [0] -> [0][1][2]...[N] - array of iterators ordered by derived class
template <typename D, uint32_t Capacity, typename Iter, typename Pred>
class HashedMultiSet {
public:
    // just pointers
    using iterator = Iter*;
    using const_iterator = const Iter*;
protected:
    struct Bucket {
        Iter* m_head{nullptr};
        uint32_t m_capacity{0};
        uint32_t m_size{0};
    };
            
    using BucketTable = std::vector<Bucket>;
    // rehash the table
    void Rehash(size_t count) noexcept;

    inline static bool Insert(Bucket& bucket, const Iter& key, const Pred& pred) noexcept;
    
    // clear table
    static void ClearTable(BucketTable& table);
    

    const HashedMultiSetSettings m_settings;
    const Pred m_compare; // hasher & equal operators
    BucketTable m_table; // buckets container
    size_t m_totalItems{0}; // keeps track of total number of items.

    HashedMultiSet(const HashedMultiSet& src) noexcept = delete;
    HashedMultiSet(HashedMultiSet&&) noexcept = delete;
    
protected:
    explicit HashedMultiSet(TupleParams<Pred>&& params) noexcept;
    ~HashedMultiSet() noexcept;
    
    // equal_range
    template <typename K>
    bool is_equal(const K& first, const K& second) const noexcept;

    bool insert(bool noRehash, const Iter& key) noexcept;
    
    // erase
    size_t erase(Iter key) noexcept;
    
    // equal_range
    template <typename K>
    std::pair<const_iterator, const_iterator> equal_range(const K& key) const noexcept;

    // find the first item by the key.
    template <typename K>
    const_iterator find(const K& key) const noexcept;
    
    static const_iterator end() noexcept { return nullptr; }
    
    // clear
    void clear() noexcept;
    
    // traverse
    void traverse() const noexcept;
};

#include "HashedMultiSet.hpp"
