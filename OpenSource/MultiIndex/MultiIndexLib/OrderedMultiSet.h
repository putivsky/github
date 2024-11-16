//
//  OrderedMultiSet.h
//  MultiIndex
//
//  Created by Yuri Putivsky on 10/12/24.
//

#pragma once

#include <set>
#include <cassert>

#define assertm(exp, msg) assert(((void)msg, exp))

// Index keeps list<Key>::iterator(s), which are essentially pointers
// therefore index nodes should be small in size, ideally just packed arrays of iterators
// to reduce the memory usage overhead.
// [0][1][2]...[M] - binary tree
// [0] -> [0][1][2]...[N] - array of iterators sorted by keys
template <uint32_t Capacity, typename Iter, typename Pred>
class OrderedMultiSet {
    struct Bucket {
        size_t m_size{0};
        Iter m_head[Capacity];
    };
    
    struct BucketNode {
        BucketNode* m_parent{nullptr};
        BucketNode* m_left{nullptr};
        BucketNode* m_right{nullptr};
        Bucket m_bucket;
        bool m_isBlack{false};
        bool m_isNull{false};
    };

private:
    // not publicaly exposed, no need to follow std iterator interface
    class iterator {
        const BucketNode* m_ptr{nullptr};
        size_t m_bucketOffset{0};

    public:
        iterator(const BucketNode* ptr, size_t bucketOffset) noexcept : m_ptr(ptr), m_bucketOffset(bucketOffset) {}

        inline iterator& operator++() noexcept {
            if (!m_ptr->m_isNull) {
                if (m_bucketOffset + 1 < m_ptr->m_bucket.m_size) {
                    // move offset
                    ++m_bucketOffset;
                } else if (!m_ptr->m_right->m_isNull) {
                    m_ptr = Min(m_ptr->m_right);
                    m_bucketOffset = 0;
                } else {
                    // Next bucket
                    BucketNode* x;
                    while (!(x = m_ptr->m_parent)->m_isNull && m_ptr == x->m_right) {
                        m_ptr = x;
                    }
                    m_ptr = x;
                    m_bucketOffset = 0;
                }
            }
            
            return *this;
        }
        
        inline iterator& operator--() noexcept {
            if (m_bucketOffset > 0) {
                // retreat offset
                --m_bucketOffset;
            } else if (m_ptr->m_isNull) {
                m_ptr = m_ptr->m_right;
                m_bucketOffset = m_ptr->m_isNull ? 0 : m_ptr->m_bucket.m_size - 1;
            } else if (!m_ptr->m_left->m_isNull) {
                m_ptr = Max(m_ptr->m_left);
                m_bucketOffset = m_ptr->m_bucket.m_size - 1;
            } else {
                BucketNode* x;
                while (!(x = m_ptr->m_parent)->m_isNull && m_ptr == x->m_left) {
                    m_ptr = x;
                }
                if (m_ptr->m_isNull) {
                    m_bucketOffset = 0;
                } else {
                    m_ptr = x;
                    m_bucketOffset = m_ptr->m_isNull ? 0 : m_ptr->m_bucket.m_size - 1;
                }
            }
            
            return *this;
        }

        inline Iter& operator*() const noexcept {
            return GetNodePtr()->m_bucket.m_head[m_bucketOffset];
        }
        
        inline size_t GetOffset() const { return m_bucketOffset; }
        inline BucketNode* GetNodePtr() const { return const_cast<BucketNode*>(m_ptr); }

        inline bool operator==(const iterator& right) const noexcept {
            return m_ptr == right.m_ptr && m_bucketOffset == right.m_bucketOffset;
        }

        inline bool operator!=(const iterator& right) const noexcept {
            return !(*this == right);
        }
    };

private:
    BucketNode* allocateNode();
    void resetHead();
    BucketNode* HeadNode() const noexcept;
    BucketNode*& Root() const noexcept ;
    BucketNode*& RMost() noexcept;
    BucketNode*& LMost() const noexcept;
    void LRotate(BucketNode* w) noexcept;
    void RRotate(BucketNode* w) noexcept;
    void Remove(BucketNode* z) noexcept;

    static BucketNode* Max(BucketNode* x) noexcept;
    static BucketNode* Min(BucketNode* x) noexcept;
    static void Destroy(BucketNode* node) noexcept;

private:
    
    const Pred m_compare;
    // m_headNode.m_parent points to root
    // m_headNode.m_left points to the left most
    // m_headNode.m_right points to the right most
    // m_headNode.m_isNull is always true, don't cast to BucketNode
    // m_headNode.m_isBlack is always true
    //    is root is null m_headNode.m_parent = m_headNode.m_left = m_headNode.m_right = &m_headNode;
    BucketNode m_headNode;
    size_t m_totalItems{0}; // keeps track of total number of items.
    
    OrderedMultiSet(const OrderedMultiSet& src) noexcept = delete;
    OrderedMultiSet(OrderedMultiSet&& src) noexcept = delete;

protected:
    explicit OrderedMultiSet(TupleParams<Pred>&& params) noexcept;
    ~OrderedMultiSet() noexcept;
 
    template <typename K>
    bool is_equal(const K& first, const K& second) const noexcept;

    // insert
    bool insert(bool, const Iter& key) noexcept;
    
    // erase
    size_t erase(Iter key) noexcept;
    
    // const version equal_range
    template <typename K>
    std::pair<iterator, iterator> equal_range(const K& key) const noexcept;

    // find the first item by the key.
    template <typename K>
    iterator find(const K& key) const noexcept;

    iterator begin() const noexcept { return iterator(LMost(), 0); }

    iterator end() const noexcept { return iterator(HeadNode(), 0); }
    
    // clear
    void clear() noexcept;
    
    // traverse
    void traverse() const noexcept;
};

#include "OrderedMultiSet.hpp"
