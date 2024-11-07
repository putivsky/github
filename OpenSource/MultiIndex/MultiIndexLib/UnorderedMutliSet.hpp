//
//  UnorderedMultiSet.hpp
//  MultiIndex
//
//  Created by Yuri Putivsky on 10/12/24.
//

template <size_t Capacity, typename Iter, typename Pred>
UnorderedMultiSet<Capacity, Iter, Pred>::UnorderedMultiSet(TupleParams<Pred>&& params) noexcept :
    m_settings(std::get<0>(params), std::get<1>(params)),
    m_compare(std::move(std::get<2>(params))) {
    m_table.resize(m_settings.minBucketCount != 0 ? m_settings.minBucketCount : 1);
}

template <size_t Capacity, typename Iter, typename Pred>
UnorderedMultiSet<Capacity, Iter, Pred>::~UnorderedMultiSet() noexcept {
    for (auto& entry : m_table) {
        ::free(entry.m_head);
    }
}

template <size_t Capacity, typename Iter, typename Pred>
template<typename I, typename K>
std::pair<I, I> UnorderedMultiSet<Capacity, Iter, Pred>::EqualKeys(const Bucket& bucket, const K& key) const noexcept {
    auto lower = std::lower_bound(bucket.m_head, bucket.m_head + bucket.m_size, key,
                               [this](const Iter& first, const K& second) -> bool { return m_compare(*first, second); });
    auto upper = std::upper_bound(lower, bucket.m_head + bucket.m_size, key,
                               [this](const K& first, const Iter& second) -> bool { return m_compare(first, *second); });
    return {lower, upper};
}

template <size_t Capacity, typename Iter, typename Pred>
template<typename I, typename K>
I UnorderedMultiSet<Capacity, Iter, Pred>::Find(const K& key) const noexcept {
    auto& bucket = m_table[m_compare(key) % m_table.size()];
    auto ptr = std::lower_bound(bucket.m_head, bucket.m_head + bucket.m_size, key,
                     [this](const Iter& first, const K& second) -> bool { return m_compare(*first, second); });
    if (ptr != (bucket.m_head + bucket.m_size) && !m_compare(**ptr, key) && !m_compare(key, **ptr)) {
        return ptr;
    }
    
    return end<I>();
}

template <size_t Capacity, typename Iter, typename Pred>
/*static*/
void UnorderedMultiSet<Capacity, Iter, Pred>::ClearTable(BucketTable& table) {
    for (auto& entry : table) {
        ::free(entry.m_head);
    }
    table.clear();
}

template <size_t Capacity, typename Iter, typename Pred>
void UnorderedMultiSet<Capacity, Iter, Pred>::Rehash(size_t count) noexcept {
    std::vector<Bucket> table;
    table.resize(count);

    // copy items
    for (auto& item : m_table) {
        for (size_t i = 0; i < item.m_size; ++i) {
            auto p = BucketInsert(table[m_compare(*item.m_head[i]) % table.size()], item.m_head[i]);
            if (!p.second) { // memory
                ClearTable(table);
                return;
            }
        }
    }
    
    // swap
    table.swap(m_table);
    ClearTable(table);
}

template <size_t Capacity, typename Iter, typename Pred>
std::pair<typename UnorderedMultiSet<Capacity, Iter, Pred>::iterator, bool>
UnorderedMultiSet<Capacity, Iter, Pred>::BucketInsert(Bucket& bucket, const Iter& key) noexcept {
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
    
    auto offset = std::lower_bound(bucket.m_head, bucket.m_head + bucket.m_size, key,
                                   [this](const Iter& first, const Iter& second) -> bool {
           return m_compare(*first, *second);
       }) - bucket.m_head;
    
    if (offset != bucket.m_size) {
        // make a room
        memmove(bucket.m_head + offset + 1, bucket.m_head + offset, sizeof(Iter) * (bucket.m_size - offset));
    }
    
    memcpy(bucket.m_head + offset, &key, sizeof(key));
    ++bucket.m_size;
    return {bucket.m_head + offset, true};
}

template <size_t Capacity, typename Iter, typename Pred>
std::pair<typename UnorderedMultiSet<Capacity, Iter, Pred>::iterator, bool>
UnorderedMultiSet<Capacity, Iter, Pred>::insert(const Iter& key) noexcept {
    if (float(m_totalItems) / m_table.size() > m_settings.maxLoadFactor) {
        Rehash(m_table.size() * 2 + 1);
    }

    auto p = BucketInsert(m_table[m_compare(*key) % m_table.size()], key);
    if (p.second) {
        ++m_totalItems;
    }
    return p;
}

template <size_t Capacity, typename Iter, typename Pred>
size_t UnorderedMultiSet<Capacity, Iter, Pred>::erase(Iter it) noexcept {
    auto& bucket = m_table[m_compare(*it) % m_table.size()];
    if (bucket.m_capacity > Capacity && bucket.m_size * 2 < Capacity) {
        bucket.m_capacity = Capacity;
        auto* memPrt = (Iter*)::realloc(bucket.m_head, bucket.m_capacity * sizeof(Iter));
        if (memPrt == nullptr) { // allocation failure
            return 0;
        }
        
        bucket.m_head = memPrt;
    }
    
    auto offset = std::lower_bound(bucket.m_head, bucket.m_head + bucket.m_size, it,
                                   [this](const Iter& first, const Iter& second) -> bool {
           return m_compare(*first, *second);
       }) - bucket.m_head;

    for (size_t idx = offset; idx < bucket.m_size; ++idx) {
        if (bucket.m_head[idx] != it) {
            continue;
        }
        
        if (offset + 1 != bucket.m_size) { // last item
            memmove(bucket.m_head + idx, bucket.m_head + idx + 1, sizeof(Iter) * (bucket.m_size - idx - 1));
        }
        --bucket.m_size;
        --m_totalItems;
        return 1;
    }
    
    return 0;
}

template <size_t Capacity, typename Iter, typename Pred>
template<typename I, typename K>
std::pair<I, I>
UnorderedMultiSet<Capacity, Iter, Pred>::equal_range(const K& key) const noexcept {
    return EqualKeys<I>(m_table[m_compare(key) % m_table.size()], key);
}

template <size_t Capacity, typename Iter, typename Pred>
template<typename I, typename K>
I
UnorderedMultiSet<Capacity, Iter, Pred>::find(const K& key) const noexcept {
    return Find<I>(key);
}

template <size_t Capacity, typename Iter, typename Pred>
void UnorderedMultiSet<Capacity, Iter, Pred>::clear() noexcept {
    ClearTable(m_table);
    m_totalItems = 0;
}

template <size_t Capacity, typename Iter, typename Pred>
void UnorderedMultiSet<Capacity, Iter, Pred>::traverse() const noexcept {
    // find value by index
    for (auto it = m_table.begin(); it != m_table.end(); ++it) {
        if (it->m_head != nullptr) {
            for (auto idx = 0; idx < it->m_size; ++idx) {
                printf("Item(unordered): %d\n", it->m_head[idx]->i);
            }
            printf(" | ");
        }
    }
}
