//
//  HashedOrderedMutliSet.hpp
//  MultiIndex
//
//  Created by Yuri Putivsky on 10/12/24.
//

template <uint32_t Capacity, typename Iter, typename Pred>
HashedOrderedMultiSet<Capacity, Iter, Pred>::HashedOrderedMultiSet(TupleParams<Pred>&& params) noexcept :
    BaseType(std::forward<TupleParams<Pred>>(params)) {
}

template <uint32_t Capacity, typename Iter, typename Pred>
template <typename I, typename K>
/*static*/
I
HashedOrderedMultiSet<Capacity, Iter, Pred>::LowerInBucket(const typename BaseType::Bucket& bucket, const K& key, const Pred& pred) noexcept {
    // find the first the same key, if any
    return std::lower_bound(bucket.m_head, bucket.m_head + bucket.m_size, key,
                                   [&pred](const Iter& first, const K& second) {
           return pred(*first, second);
       });
}

template <uint32_t Capacity, typename Iter, typename Pred>
template <typename I, typename K>
/*static*/
std::pair<I, I>
HashedOrderedMultiSet<Capacity, Iter, Pred>::EqualKeys(const typename BaseType::Bucket& bucket, const K& key, const Pred& pred) noexcept {
    auto lower = LowerInBucket<I>(bucket, key, pred);
    auto upper = std::upper_bound(decltype(bucket.m_head)(lower), bucket.m_head + bucket.m_size, key,
                               [&pred](const K& first, const Iter& second) -> bool { return pred(first, *second); });
    return {lower, upper};
}

template <uint32_t Capacity, typename Iter, typename Pred>
template <typename K>
/*static*/
bool HashedOrderedMultiSet<Capacity, Iter, Pred>::IsEqual(const K& first, const K& second, const Pred& pred) noexcept {
    return !pred(first, second) && !pred(second, first);
}
