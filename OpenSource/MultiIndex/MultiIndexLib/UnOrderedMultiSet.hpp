//
//  UnOrderedMultiSet.hpp
//  MultiIndex
//
//  Created by Yuri Putivsky on 10/12/24.
//

template <uint32_t Capacity, typename Iter, typename Pred>
UnOrderedMultiSet<Capacity, Iter, Pred>::UnOrderedMultiSet(TupleParams<Pred>&& params) noexcept :
    BaseType(std::forward<TupleParams<Pred>>(params)) {
}

template <uint32_t Capacity, typename Iter, typename Pred>
UnOrderedMultiSet<Capacity, Iter, Pred>::~UnOrderedMultiSet() noexcept {
}

template <uint32_t Capacity, typename Iter, typename Pred>
template<typename I, typename K>
/*static*/
std::pair<I, I>
UnOrderedMultiSet<Capacity, Iter, Pred>::EqualKeys(const typename BaseType::Bucket& bucket, const K& key, const Pred& pred) noexcept {
    size_t lowerIdx = LowerBound<I, K>(bucket, key, pred) - bucket.m_head;
    size_t upperIdx = lowerIdx;
    for (; upperIdx < bucket.m_size && pred(key, *bucket.m_head[upperIdx]); ++upperIdx);

    return {bucket.m_head + lowerIdx, bucket.m_head + upperIdx};
}

template <uint32_t Capacity, typename Iter, typename Pred>
template<typename I, typename K>
/*static*/
I UnOrderedMultiSet<Capacity, Iter, Pred>::LowerBound(const typename BaseType::Bucket& bucket, const K& key, const Pred& pred) noexcept {
    // find the first the same key, if any
    size_t lowerIdx = 0;
    for (; lowerIdx < bucket.m_size && !pred(key, *bucket.m_head[lowerIdx]); ++lowerIdx);

    return {bucket.m_head + lowerIdx};
}

template <uint32_t Capacity, typename Iter, typename Pred>
template<typename K>
/*static*/
bool UnOrderedMultiSet<Capacity, Iter, Pred>::IsEqual(const K& first, const K& second, const Pred& pred) noexcept {
    return pred(first, second);
}
