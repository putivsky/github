//
//  HashedMultiSet.hpp
//  MultiIndex
//
//  Created by Yuri Putivsky on 10/12/24.
//

template <typename D, uint32_t Capacity, typename Iter, typename Pred>
HashedMultiSet<D, Capacity, Iter, Pred>::HashedMultiSet(TupleParams<Pred>&& params) noexcept :
    m_settings(std::get<0>(params), std::get<1>(params)),
    m_compare(std::move(std::get<2>(params))) {
    m_table.resize(m_settings.minBucketCount != 0 ? m_settings.minBucketCount : 1);
}

template <typename D, uint32_t Capacity, typename Iter, typename Pred>
HashedMultiSet<D, Capacity, Iter, Pred>::~HashedMultiSet() noexcept {
    ClearTable(m_table);
}

template <typename D, uint32_t Capacity, typename Iter, typename Pred>
/*static*/
void HashedMultiSet<D, Capacity, Iter, Pred>::ClearTable(BucketTable& table) {
    for (auto& entry : table) {
        ::free(entry.m_head);
    }
    table.clear();
}

template <typename D, uint32_t Capacity, typename Iter, typename Pred>
void HashedMultiSet<D, Capacity, Iter, Pred>::Rehash(size_t count) noexcept {
    std::vector<Bucket> table;
    table.resize(count);

    // copy items
    for (auto& item : m_table) {
        for (size_t i = 0; i < item.m_size; ++i) {
            if (!Insert(table[m_compare(*item.m_head[i]) % table.size()], item.m_head[i], m_compare)) { // memory
                ClearTable(table);
                return;
            }
        }
    }
    
    // swap
    table.swap(m_table);
    ClearTable(table);
}

template <typename D, uint32_t Capacity, typename Iter, typename Pred>
bool
HashedMultiSet<D, Capacity, Iter, Pred>::Insert(Bucket& bucket, const Iter& key, const Pred& pred) noexcept {
    if (bucket.m_head == nullptr) {
        bucket.m_capacity = Capacity;
        bucket.m_head = (Iter*)::malloc(bucket.m_capacity * sizeof(Iter));
        bucket.m_size = 0;
    } else if (bucket.m_capacity == bucket.m_size) {
        bucket.m_capacity = bucket.m_size * 2;
        auto* memPrt = (Iter*)::realloc(bucket.m_head, bucket.m_capacity * sizeof(Iter));
        // allocation failure
        if (memPrt == nullptr) {
            return false;
        }
        
        bucket.m_head = memPrt;
    }
    
    // find the first same key, if any
    auto ptr = D::template LowerInBucket<iterator>(bucket, *key, pred);

    if (ptr != bucket.m_head + bucket.m_size) {
        // make a room
        memmove(ptr + 1, ptr, sizeof(Iter) * (bucket.m_size - (ptr - bucket.m_head)));
    }
    
    memcpy(ptr, &key, sizeof(key));
    ++bucket.m_size;
        
    return true;
}

//////////////////////////////
template <typename D, uint32_t Capacity, typename Iter, typename Pred>
template <typename K>
bool HashedMultiSet<D, Capacity, Iter, Pred>::is_equal(const K& first, const K& second) const noexcept {
    return D::IsEqual(first, second, m_compare);
}

template <typename D, uint32_t Capacity, typename Iter, typename Pred>
bool
HashedMultiSet<D, Capacity, Iter, Pred>::insert(bool noRehash, const Iter& key) noexcept {
    if (!noRehash && float(m_totalItems) / m_table.size() > m_settings.maxLoadFactor) {
        Rehash(m_table.size() * 2 + 1);
    }

    bool res = Insert(m_table[m_compare(*key) % m_table.size()], key, m_compare);
    if (res) {
        ++m_totalItems;
    }
    
    return res;
}

template <typename D, uint32_t Capacity, typename Iter, typename Pred>
size_t HashedMultiSet<D, Capacity, Iter, Pred>::erase(Iter it) noexcept {
    auto& bucket = m_table[m_compare(*it) % m_table.size()];

    if (bucket.m_head != nullptr) {
        if (bucket.m_capacity > Capacity && bucket.m_size * 2 < Capacity) {
            bucket.m_capacity = Capacity;
            auto* memPrt = (Iter*)::realloc(bucket.m_head, bucket.m_capacity * sizeof(Iter));
            if (memPrt == nullptr) { // allocation failure
                return 0;
            }
            
            bucket.m_head = memPrt;
        }
        
        for (auto p = D::template EqualKeys<iterator>(bucket, *it, m_compare); p.first != p.second; ++p.first) {
            if (*p.first != it) {
                continue;
            }
            
            size_t offset = p.first - bucket.m_head;
            
            if (offset + 1 != bucket.m_size) { // last item
                memmove(p.first, p.first + 1, sizeof(Iter) * (bucket.m_size - offset - 1));
            }
            
            --bucket.m_size;
            --m_totalItems;
            return 1;
        }
    }
    
    return 0;
}

template <typename D, uint32_t Capacity, typename Iter, typename Pred>
template <typename K>
std::pair<typename HashedMultiSet<D, Capacity, Iter, Pred>::const_iterator, typename HashedMultiSet<D, Capacity, Iter, Pred>::const_iterator>
HashedMultiSet<D, Capacity, Iter, Pred>::equal_range(const K& key) const noexcept {
    auto& bucket = m_table[m_compare(key) % m_table.size()];
    
    if (bucket.m_head == nullptr) {
        return {end(), end()};
    }
    
    return D::template EqualKeys<const_iterator>(bucket, key, m_compare);
}

template <typename D, uint32_t Capacity, typename Iter, typename Pred>
template <typename K>
typename HashedMultiSet<D, Capacity, Iter, Pred>::const_iterator
HashedMultiSet<D, Capacity, Iter, Pred>::find(const K& key) const noexcept {
    auto& bucket = m_table[m_compare(key) % m_table.size()];
    if (bucket.m_head != nullptr) {
        auto ptr = D::template LowerInBucket<const_iterator>(bucket, key, m_compare);
        
        if (ptr != bucket.m_head + bucket.m_size && D::template IsEqual<K>(key, **ptr, m_compare)) {
            return ptr;
        }
    }
    
    return end();
}

template <typename D, uint32_t Capacity, typename Iter, typename Pred>
void HashedMultiSet<D, Capacity, Iter, Pred>::clear() noexcept {
    ClearTable(m_table);
    m_totalItems = 0;
}

template <typename D, uint32_t Capacity, typename Iter, typename Pred>
void HashedMultiSet<D, Capacity, Iter, Pred>::traverse() const noexcept {
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
