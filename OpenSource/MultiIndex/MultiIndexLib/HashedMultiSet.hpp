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
    for (auto& entry : m_table) {
        ::free(entry.m_head);
    }
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
            auto p = D::BucketInsert(table[m_compare(*item.m_head[i]) % table.size()], item.m_head[i], m_compare);
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

template <typename D, uint32_t Capacity, typename Iter, typename Pred>
template<typename K>
bool HashedMultiSet<D, Capacity, Iter, Pred>::is_equal(const K& first, const K& second) const noexcept {
    return D::template IsEqual<K>(first, second, m_compare);
}

template <typename D, uint32_t Capacity, typename Iter, typename Pred>
std::pair<typename HashedMultiSet<D, Capacity, Iter, Pred>::iterator, bool>
HashedMultiSet<D, Capacity, Iter, Pred>::insert(bool noRehash, const Iter& key) noexcept {
    if (!noRehash && float(m_totalItems) / m_table.size() > m_settings.maxLoadFactor) {
        Rehash(m_table.size() * 2 + 1);
    }

    auto p = D::BucketInsert(m_table[m_compare(*key) % m_table.size()], key, m_compare);
    if (p.second) {
        ++m_totalItems;
    }
    return p;
}

template <typename D, uint32_t Capacity, typename Iter, typename Pred>
size_t HashedMultiSet<D, Capacity, Iter, Pred>::erase(Iter it) noexcept {
    auto& bucket = m_table[m_compare(*it) % m_table.size()];
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
    
    return 0;
}

template <typename D, uint32_t Capacity, typename Iter, typename Pred>
template<typename I, typename K>
std::pair<I, I>
HashedMultiSet<D, Capacity, Iter, Pred>::equal_range(const K& key) const noexcept {
    return D::template EqualKeys<I>(m_table[m_compare(key) % m_table.size()], key, m_compare);
}

template <typename D, uint32_t Capacity, typename Iter, typename Pred>
template<typename I, typename K>
I
HashedMultiSet<D, Capacity, Iter, Pred>::find(const K& key) const noexcept {
    return D::template Find<I>(m_table[m_compare(key) % m_table.size()], key, m_compare);
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
