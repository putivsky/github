//
//  OrderedMutliSet.h
//  MultiIndex
//
//  Created by Yuri Putivsky on 10/12/24.
//

#pragma once

#include <set>
#include <cassert>

#define assertm(exp, msg) assert(((void)msg, exp))

// Index keeps list<Key>::iterator(s), which are essentially pointers
// therfore index nodes should be small in size, ideally just packed arrays of iterators
// to reduce the memory usage overhead.
// [0][1][2]...[M] - binary tree
// [0] -> [0][1][2]...[N] - array of iterators sorted by keys
template <size_t Capacity, typename Iter, typename Pred>
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

public:
    class const_iterator;
    class iterator;

    class base_iterator {
        friend class const_iterator;
        friend class iterator;

        BucketNode* m_ptr{nullptr};
        size_t m_bucketOffset{0};

        explicit base_iterator(BucketNode* ptr, size_t bucketOffset) noexcept : m_ptr(ptr), m_bucketOffset(bucketOffset) {}

        template<typename I>
        inline I& operator++() noexcept {
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
            
            return static_cast<I&>(*this);
        }
        
        template<typename I>
        inline I operator++(int) noexcept {
            I tmp(*this);
            ++(*this);
            return tmp;
        }

        template<typename I>
        inline I& operator--() noexcept {
            if (m_bucketOffset > 0) {
                // retreat offset
                --m_bucketOffset;
            } else if (m_ptr->m_isNull) {
                m_ptr = m_ptr->m_right;
                m_bucketOffset = m_ptr->m_bucket.m_size - 1;
            } else if (!m_ptr->m_left->m_iNull) {
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
                    m_bucketOffset = m_ptr->m_bucket.m_size - 1;
                }
            }
            
            return static_cast<I&>(*this);
        }
        
        template<typename I>
        inline I operator--(int) noexcept {
            I tmp(*this);
            --(*this);
            return tmp;
        }

    public:
        inline Iter& operator*() noexcept {
            return this->m_ptr->m_bucket.m_head[this->m_bucketOffset];
        }
        
        inline Iter* operator->() noexcept {
            return m_ptr->m_bucket.m_head + this->m_bucketOffset;
        }
        
        inline const Iter& operator*() const noexcept {
            return m_ptr->m_bucket.m_head[this->m_bucketOffset];
        }
        
        inline const Iter* operator->() const noexcept {
            return m_ptr->m_bucket.m_head + this->m_bucketOffset;
        }

        inline BucketNode* GetNodePtr() const { return m_ptr; }
        inline size_t GetOffset() const { return m_bucketOffset; }

        inline bool operator==(const base_iterator& right) const noexcept {
            return m_ptr == right.m_ptr && m_bucketOffset == right.m_bucketOffset;
        }

        inline bool operator!=(const base_iterator& right) const noexcept {
            return !(*this == right);
        }
    };
    
    class iterator : public base_iterator {
    public:
        explicit iterator(BucketNode* ptr, size_t bucketOffset) noexcept : base_iterator(ptr, bucketOffset) {}

        inline iterator& operator++() noexcept {
            return base_iterator::template operator++<iterator>();
        }
        
        inline iterator operator++(int) noexcept {
            return base_iterator::template operator++<iterator>(0);
        }

        inline iterator& operator--() noexcept {
            return base_iterator::template operator--<iterator>();
        }
        
        inline iterator operator--(int) noexcept {
            return base_iterator::template operator--<iterator>(0);
        }

        inline friend bool operator==(const iterator& left, const iterator& right) noexcept {
            return static_cast<const base_iterator&>(left).operator==(static_cast<const base_iterator&>(right));
        }
        
        inline friend bool operator!=(const iterator& left, const iterator& right) noexcept {
          return !(left == right);
        }
    };
    
    class const_iterator : public iterator {
    public:
        explicit const_iterator(BucketNode* ptr, size_t bucketOffset) noexcept : iterator(ptr, bucketOffset) {}

        const_iterator(iterator it) noexcept : iterator(it.m_ptr, it.m_bucketOffset) {}

        inline const_iterator& operator++() noexcept {
            return base_iterator::template operator++<const_iterator>();
        }
        
        inline const_iterator operator++(int) noexcept {
            return base_iterator::template operator++<const_iterator>(0);
        }

        inline const_iterator& operator--() noexcept {
            return base_iterator::template operator--<const_iterator>();
        }
        
        inline const_iterator operator--(int) noexcept {
            return base_iterator::template operator--<const_iterator>(0);
        }

        inline friend bool operator==(const const_iterator& left, const const_iterator& right) noexcept {
            return static_cast<const base_iterator&>(left).operator==(static_cast<const base_iterator&>(right));
                //return left.m_ptr == right.m_ptr && left.m_bucketOffset == right.m_bucketOffset;
        }
        
        inline friend bool operator!=(const const_iterator& left, const const_iterator& right) noexcept {
            return !(left == right);
        }
    };

protected:
    const Pred& key_comp() const noexcept { return m_compare; }
    
private:
    BucketNode* allocateNode() const;
    void resetHead();
    BucketNode*& Root() const noexcept ;
    BucketNode*& RMost() const noexcept;
    BucketNode*& LMost() const noexcept;
    BucketNode* HeadNode() const noexcept;
    static BucketNode* Max(BucketNode* x) noexcept;
    static BucketNode* Min(BucketNode* x) noexcept;
    void LRotate(BucketNode* w) const noexcept;
    void RRotate(BucketNode* w) const noexcept;
    void Remove(BucketNode* z) noexcept;
    static void Destroy(BucketNode* node) noexcept;
    template<typename K>
    iterator LowerBound(const K& key) const noexcept;
    template <typename K>
    iterator UpperBound(const K& key) const noexcept;
    template< typename I, typename K>
    I Find(const K& key) const noexcept;
    template< typename I, typename K>
    std::pair<I, I> EqualRange(const K& key) const noexcept;

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
    OrderedMultiSet(TupleParams<Pred>&& params) noexcept;
    ~OrderedMultiSet() noexcept;
 
    // insert
    std::pair<iterator, bool> insert(const Iter& key) noexcept;
    
    // erase
    size_t erase(Iter key) noexcept;
    
    // const version equal_range
    template<typename I, typename K>
    std::pair<I, I> equal_range(const K& key) const noexcept;

    // find the first item by the key.
    template<typename I, typename K>
    I find(const K& key) const noexcept;

    template<typename I>
    I begin() const noexcept { return I(LMost(), 0); }

    template<typename I>
    I end() const noexcept { return I(HeadNode(), 0); }
    
    // clear
    void clear() noexcept;
    
    // traverse
    void traverse() const noexcept;
};

#include "OrderedMutliSet.hpp"
