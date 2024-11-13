//
//  HashedOrderedMutliSet.h
//  MultiIndex
//
//  Created by Yuri Putivsky on 10/12/24.
//

#pragma once

#include "HashedMultiSet.h"

// Index keeps list<Key>::iterator(s), which are essentially pointers
// therfore index nodes should be small in size, ideally just packed arrays of iterators
// to reduce the memory usage overhead.
// [0][1][2]...[M] - buckets
// [0] -> [0][1][2]...[N] - array of iterators ordered by keys
template <uint32_t Capacity, typename Iter, typename Pred>
class HashedOrderedMultiSet : public HashedMultiSet<HashedOrderedMultiSet<Capacity, Iter, Pred>, Capacity, Iter, Pred> {
public:
    using ThisType = HashedOrderedMultiSet<Capacity, Iter, Pred>;
    using BaseType = HashedMultiSet<ThisType, Capacity, Iter, Pred>;

    template<typename I, typename K>
    static std::pair<I, I> EqualKeys(
        const typename BaseType::Bucket& bucket,
        const K& key,
        const Pred& pred) noexcept;
    // inserts new key into bucket and keeps the same keys grouped (not sorted) within bucket.
    static std::pair<typename BaseType::iterator, bool> BucketInsert(
        typename BaseType::Bucket& bucket,
        const Iter& key,
        const Pred& pred) noexcept;
    
    template<typename I, typename K>
    static I Find(
        const typename BaseType::Bucket& bucket,
        const K& key,
        const Pred& pred) noexcept;

    template<typename K>
    static bool IsEqual(const K& first, const K& second, const Pred& pred) noexcept;

private:
    HashedOrderedMultiSet(const HashedOrderedMultiSet& src) noexcept = delete;
    HashedOrderedMultiSet(HashedOrderedMultiSet&&) noexcept = delete;
    
protected:
    explicit HashedOrderedMultiSet(TupleParams<Pred>&& params) noexcept;
    ~HashedOrderedMultiSet() noexcept;
};

#include "HashedOrderedMultiSet.hpp"
