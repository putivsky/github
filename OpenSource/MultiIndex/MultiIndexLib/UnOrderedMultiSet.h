//
//  UnOrderedMultiSet.h
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
// [0] -> [0][1][2]...[N] - array of iterators groupped by keys
template <uint32_t Capacity, typename Iter, typename Pred>
class UnOrderedMultiSet : public HashedMultiSet<UnOrderedMultiSet<Capacity, Iter, Pred>, Capacity, Iter, Pred> {
public:
    using ThisType = UnOrderedMultiSet<Capacity, Iter, Pred>;
    using BaseType = HashedMultiSet<ThisType, Capacity, Iter, Pred>;

    template<typename I, typename K>
    inline static std::pair<I, I> EqualKeys(
        const typename BaseType::Bucket& bucket,
        const K& key,
        const Pred& pred) noexcept;
    
    template<typename I, typename K>
    inline static I LowerBound(
        const typename BaseType::Bucket& bucket,
        const K& key,
        const Pred& pred) noexcept;

    template<typename K>
    inline static bool IsEqual(const K& first, const K& second, const Pred& pred) noexcept;

private:
    explicit UnOrderedMultiSet(const UnOrderedMultiSet& src) noexcept = delete;
    UnOrderedMultiSet(UnOrderedMultiSet&&) noexcept = delete;
    
protected:
    UnOrderedMultiSet(TupleParams<Pred>&& params) noexcept;
    ~UnOrderedMultiSet() noexcept;
};

#include "UnOrderedMultiSet.hpp"
