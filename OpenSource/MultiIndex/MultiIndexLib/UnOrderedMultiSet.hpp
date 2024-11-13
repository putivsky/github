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
    
    // find the first the same key, if any
    size_t lowerIdx = 0;
    for (; lowerIdx < bucket.m_size && !pred(key, *bucket.m_head[lowerIdx]); ++lowerIdx);
    size_t upperIdx = lowerIdx;
    for (; upperIdx < bucket.m_size && pred(key, *bucket.m_head[upperIdx]); ++upperIdx);

    return {bucket.m_head + lowerIdx, bucket.m_head + upperIdx};
}

template <uint32_t Capacity, typename Iter, typename Pred>
template<typename I, typename K>
/*static*/
I
UnOrderedMultiSet<Capacity, Iter, Pred>::Find(const typename BaseType::Bucket& bucket, const K& key, const Pred& pred) noexcept {
    
    // find the first the same key, if any
    size_t lowerIdx = 0;
    for (; lowerIdx < bucket.m_size && !pred(key, *bucket.m_head[lowerIdx]); ++lowerIdx);
        
    if (lowerIdx != bucket.m_size) {
        return bucket.m_head + lowerIdx;
    }
    
    return BaseType::template end<I>();
}

template <uint32_t Capacity, typename Iter, typename Pred>
template<typename K>
/*static*/
bool UnOrderedMultiSet<Capacity, Iter, Pred>::IsEqual(const K& first, const K& second, const Pred& pred) noexcept {
    return pred(first, second);
}

template <uint32_t Capacity, typename Iter, typename Pred>
std::pair<typename UnOrderedMultiSet<Capacity, Iter, Pred>::BaseType::iterator, bool>
UnOrderedMultiSet<Capacity, Iter, Pred>::BucketInsert(typename BaseType::Bucket& bucket, const Iter& key, const Pred& pred) noexcept {
    if (bucket.m_head == nullptr) {
        bucket.m_capacity = Capacity;
        bucket.m_head = (Iter*)::malloc(bucket.m_capacity * sizeof(Iter));
        bucket.m_size = 0;
    } else if (bucket.m_capacity == bucket.m_size) {
        bucket.m_capacity = bucket.m_size * 2;
        auto* memPrt = (Iter*)::realloc(bucket.m_head, bucket.m_capacity * sizeof(Iter));
        // TODO - allocation failure
        if (memPrt == nullptr) {
            return {nullptr, false};
        }
        
        bucket.m_head = memPrt;
    }
    
    // find the first the same key, if any
    auto p = EqualKeys<typename BaseType::iterator>(bucket, *key, pred);
    
    if (p.second != bucket.m_head + bucket.m_size) {
        // make a room
        memmove(p.second + 1, p.second, sizeof(Iter) * (bucket.m_size - (p.second - bucket.m_head)));
    }
    
    memcpy(p.second, &key, sizeof(key));
    ++bucket.m_size;
    return {p.second, true};
}
