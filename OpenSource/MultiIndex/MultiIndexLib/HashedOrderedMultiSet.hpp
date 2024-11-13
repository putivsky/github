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
HashedOrderedMultiSet<Capacity, Iter, Pred>::~HashedOrderedMultiSet() noexcept {
}

template <uint32_t Capacity, typename Iter, typename Pred>
template<typename I, typename K>
/*static*/
std::pair<I, I>
HashedOrderedMultiSet<Capacity, Iter, Pred>::EqualKeys(const typename BaseType::Bucket& bucket, const K& key, const Pred& pred) noexcept {
    auto lower = std::lower_bound(bucket.m_head, bucket.m_head + bucket.m_size, key,
                               [&pred](const Iter& first, const K& second) -> bool { return pred(*first, second); });
    auto upper = std::upper_bound(lower, bucket.m_head + bucket.m_size, key,
                               [&pred](const K& first, const Iter& second) -> bool { return pred(first, *second); });
    return {lower, upper};
}

template <uint32_t Capacity, typename Iter, typename Pred>
template<typename I, typename K>
/*static*/
I HashedOrderedMultiSet<Capacity, Iter, Pred>::Find(const typename BaseType::Bucket& bucket, const K& key, const Pred& pred) noexcept {
    auto ptr = std::lower_bound(bucket.m_head, bucket.m_head + bucket.m_size, key,
                                   [&pred](const Iter& first, const K& second) -> bool {
           return pred(*first, second);
       });

    if (ptr != bucket.m_head + bucket.m_size && !pred(key, **ptr) && !pred(**ptr, key)) {
        return ptr;
    }
    
    return BaseType::template end<I>();
}

template <uint32_t Capacity, typename Iter, typename Pred>
template<typename K>
/*static*/
bool HashedOrderedMultiSet<Capacity, Iter, Pred>::IsEqual(const K& first, const K& second, const Pred& pred) noexcept {
    return !pred(first, second) && !pred(second, first);
}

template <uint32_t Capacity, typename Iter, typename Pred>
std::pair<typename HashedOrderedMultiSet<Capacity, Iter, Pred>::BaseType::iterator, bool>
/*static*/
HashedOrderedMultiSet<Capacity, Iter, Pred>::BucketInsert(typename BaseType::Bucket& bucket, const Iter& key, const Pred& pred) noexcept {
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
    
    auto ptr = std::lower_bound(bucket.m_head, bucket.m_head + bucket.m_size, key,
                                   [&pred](const Iter& first, const Iter& second) -> bool {
           return pred(*first, *second);
       });

    if (ptr != bucket.m_head + bucket.m_size) {
        // make a room
        memmove(ptr + 1, ptr, sizeof(Iter) * (bucket.m_size - (ptr - bucket.m_head)));
    }
    
    memcpy(ptr, &key, sizeof(key));
    ++bucket.m_size;
        
    return {ptr, true};
}
