//
//  OrderedMultiSet.hpp
//  MultiIndex
//
//  Created by Yuri Putivsky on 10/12/24.
//

template <uint32_t Capacity, typename Iter, typename Pred>
OrderedMultiSet<Capacity, Iter, Pred>::OrderedMultiSet(TupleParams<Pred>&& params) noexcept :
    m_compare(std::move(std::get<2>(params))) {
    resetHead();
}

template <uint32_t Capacity, typename Iter, typename Pred>
OrderedMultiSet<Capacity, Iter, Pred>::~OrderedMultiSet() noexcept {
    Destroy(Root());
}

template <uint32_t Capacity, typename Iter, typename Pred>
typename OrderedMultiSet<Capacity, Iter, Pred>::BucketNode*& OrderedMultiSet<Capacity, Iter, Pred>::Root() const noexcept {
    return const_cast<BucketNode*&>(m_headNode.m_parent);
}

template <uint32_t Capacity, typename Iter, typename Pred>
typename OrderedMultiSet<Capacity, Iter, Pred>::BucketNode*& OrderedMultiSet<Capacity, Iter, Pred>::RMost() const noexcept { // return rightmost node in nonmutable tree
    return const_cast<BucketNode*&>(m_headNode.m_right);
}

template <uint32_t Capacity, typename Iter, typename Pred>
typename OrderedMultiSet<Capacity, Iter, Pred>::BucketNode*& OrderedMultiSet<Capacity, Iter, Pred>::LMost() const noexcept { // return leftmost node in nonmutable tree
    return const_cast<BucketNode*&>(m_headNode.m_left);
}

template <uint32_t Capacity, typename Iter, typename Pred>
typename OrderedMultiSet<Capacity, Iter, Pred>::BucketNode* OrderedMultiSet<Capacity, Iter, Pred>::HeadNode() const noexcept {
    return const_cast<BucketNode*>(reinterpret_cast<const BucketNode*>(&m_headNode));
}

template <uint32_t Capacity, typename Iter, typename Pred>
/* static*/
typename OrderedMultiSet<Capacity, Iter, Pred>::BucketNode* OrderedMultiSet<Capacity, Iter, Pred>::Max(BucketNode* x) noexcept {    // return rightmost node in subtree at x
    while (!x->m_right->m_isNull) {
        x = x->m_right;
    }
    return x;
}

template <uint32_t Capacity, typename Iter, typename Pred>
/*static*/
typename OrderedMultiSet<Capacity, Iter, Pred>::BucketNode* OrderedMultiSet<Capacity, Iter, Pred>::Min(BucketNode* x) noexcept {    // return leftmost node in subtree at x
    while (!x->m_left->m_isNull) {
        x = x->m_left;
    }
    return x;
}

template <uint32_t Capacity, typename Iter, typename Pred>
void OrderedMultiSet<Capacity, Iter, Pred>::LRotate(BucketNode* w) const noexcept {
    BucketNode* x = w->m_right;
    w->m_right = x->m_left;
    if (!x->m_left->m_isNull) {
        x->m_left->m_parent = w;
    }
    x->m_parent = w->m_parent;
    if (w == Root()) {
        Root() = x;
    } else if (w == w->m_parent->m_left) {
        w->m_parent->m_left = x;
    } else {
        w->m_parent->m_right = x;
    }
    
    x->m_left = w;
    w->m_parent = x;
}

template <uint32_t Capacity, typename Iter, typename Pred>
void OrderedMultiSet<Capacity, Iter, Pred>::RRotate(BucketNode* w) const noexcept {
    BucketNode* x = w->m_left;
    w->m_left = x->m_right;
    if (!x->m_right->m_isNull) {
        x->m_right->m_parent = w;
    }
    x->m_parent = w->m_parent;
    if (w == Root()) {
        Root() = x;
    } else if (w == w->m_parent->m_right) {
        w->m_parent->m_right = x;
    } else {
        w->m_parent->m_left = x;
    }
    
    x->m_right = w;
    w->m_parent = x;
}
    
template <uint32_t Capacity, typename Iter, typename Pred>
void OrderedMultiSet<Capacity, Iter, Pred>::Remove(BucketNode* z) noexcept {
    BucketNode* rcolorNode;    // the node to recolor as needed
    BucketNode* rcolorNodeParent;    // parent of rcolorNode (which may be nil)
    BucketNode* x = z;
    
    if (x->m_left->m_isNull) {
        rcolorNode = x->m_right;    // stitch up right subtree
    } else if (x->m_right->m_isNull) {
        rcolorNode = x->m_left;    // stitch up left subtree
    } else {    // two subtrees, must lift successor node to replace erased
        iterator next(z, z->m_bucket.m_size - 1);
        ++next;
        x = next.GetNodePtr();    // x is successor node
        rcolorNode = x->m_right;    // rcolorNode is only subtree
    }
    
    if (x == z) {    // at most one subtree, relink it
        rcolorNodeParent = z->m_parent;
        if (!rcolorNode->m_isNull) {
            rcolorNode->m_parent = rcolorNodeParent;    // link up
        }
        
        if (Root() == z) {
            Root() = rcolorNode;    // link down from root
        } else if (rcolorNodeParent->m_left == z) {
            rcolorNodeParent->m_left = rcolorNode;    // link down to left
        } else {
            rcolorNodeParent->m_right = rcolorNode;    // link down to right
        }
        
        if (LMost() == z) {
            LMost() = rcolorNode->m_isNull
            ? rcolorNodeParent    // smallest is parent of erased node
            : Min(rcolorNode);    // smallest in relinked subtree
        }
        
        if (RMost() == z) {
            RMost() = rcolorNode->m_isNull
            ? rcolorNodeParent    // largest is parent of erased node
            : Max(rcolorNode);    // largest in relinked subtree
        }
    } else {    // erased has two subtrees, _Pnode is successor to erased
        z->m_left->m_parent = x; // link left up
        x->m_left = z->m_left; // link successor down

        if (x == z->m_right) {
            rcolorNodeParent = x;    // successor is next to erased
        } else {    // successor further down, link in place of erased
            rcolorNodeParent = x->m_parent;
            if (!rcolorNode->m_isNull) {
                rcolorNode->m_parent = rcolorNodeParent;    // link fix up
            }
            rcolorNodeParent->m_left = rcolorNode;    // link fix down
            x->m_right = z->m_right;
            z->m_right->m_parent = x; // right up
        }

        if (Root() == z) {
            Root() = x;    // link down from root
        } else if (z->m_parent->m_left == z) {
            z->m_parent->m_left = x;    // link down to left
        } else {
            z->m_parent->m_right = x; // link down to right
        }

        x->m_parent = z->m_parent; // link successor up
        std::swap(x->m_isBlack, z->m_isBlack);// recolor it
    }

    if (z->m_isBlack) {    // erasing black link, must recolor/rebalance tree
        for (; rcolorNode != Root() && rcolorNode->m_isBlack; rcolorNodeParent = rcolorNode->m_parent) {
            if (rcolorNode == rcolorNodeParent->m_left) {    // fixup left subtree
                x = rcolorNodeParent->m_right;
                if (!x->m_isBlack) {    // rotate red up from right subtree
                    x->m_isBlack = true;
                    rcolorNodeParent->m_isBlack = false;
                    LRotate(rcolorNodeParent);
                    x = rcolorNodeParent->m_right;
                }
                
                if (x->m_isNull) {
                    rcolorNode = rcolorNodeParent;    // shouldn't happen
                } else if (x->m_left->m_isBlack && x->m_right->m_isBlack) {    // redden right subtree with black children
                    x->m_isBlack = false;
                    rcolorNode = rcolorNodeParent;
                } else {    // must rearrange right subtree
                    if (x->m_right->m_isBlack) {    // rotate red up from left sub-subtree
                        x->m_left->m_isBlack = true;
                        x->m_isBlack = false;
                        RRotate(x);
                        x= rcolorNodeParent->m_right;
                    }
                    
                    x->m_isBlack = rcolorNodeParent->m_isBlack;
                    rcolorNodeParent->m_isBlack = true;
                    x->m_right->m_isBlack = true;
                    LRotate(rcolorNodeParent);
                    break;    // tree now recolored/rebalanced
                }
            } else {    // fixup right subtree
                x = rcolorNodeParent->m_left;
                if (!x->m_isBlack) {    // rotate red up from left subtree
                    x->m_isBlack = true;
                    rcolorNodeParent->m_isBlack = false;
                    RRotate(rcolorNodeParent);
                    x = rcolorNodeParent->m_left;
                }
                if (x->m_isNull) {
                    rcolorNode = rcolorNodeParent;    // shouldn't happen
                } else if (x->m_right->m_isBlack && x->m_left->m_isBlack) {    // redden left subtree with black children
                    x->m_isBlack = false;
                    rcolorNode = rcolorNodeParent;
                } else {    // must rearrange left subtree
                    if (x->m_left->m_isBlack) {    // rotate red up from right sub-subtree
                        x->m_right->m_isBlack = true;
                        x->m_isBlack = false;
                        LRotate(x);
                        x = rcolorNodeParent->m_left;
                    }
                    
                    x->m_isBlack = rcolorNodeParent->m_isBlack;
                    rcolorNodeParent->m_isBlack = true;
                    x->m_left->m_isBlack = true;
                    RRotate(rcolorNodeParent);
                    break;    // tree now recolored/rebalanced
                }
            }
        }
        rcolorNode->m_isBlack = true;
    }
}

template <uint32_t Capacity, typename Iter, typename Pred>
/*static*/
void OrderedMultiSet<Capacity, Iter, Pred>::Destroy(OrderedMultiSet<Capacity, Iter, Pred>::BucketNode* node) noexcept {
    if (!node->m_isNull) {
        Destroy(node->m_left);
        Destroy(node->m_right);
        delete node;
    }
}

template <uint32_t Capacity, typename Iter, typename Pred>
typename OrderedMultiSet<Capacity, Iter, Pred>::BucketNode* OrderedMultiSet<Capacity, Iter, Pred>::allocateNode() const {
    BucketNode* newNode = new BucketNode;
    newNode->m_left = HeadNode();
    newNode->m_right = HeadNode();
    newNode->m_parent = HeadNode();
    newNode->m_isBlack = false;
    newNode->m_isNull = false;
    return newNode;
}

template <uint32_t Capacity, typename Iter, typename Pred>
void OrderedMultiSet<Capacity, Iter, Pred>::resetHead() {
    m_headNode.m_parent = &m_headNode;
    m_headNode.m_left = &m_headNode;
    m_headNode.m_right = &m_headNode;
    m_headNode.m_isBlack = true;
    m_headNode.m_isNull = true;
}

template <uint32_t Capacity, typename Iter, typename Pred>
template<typename K>
typename OrderedMultiSet<Capacity, Iter, Pred>::iterator OrderedMultiSet<Capacity, Iter, Pred>::LowerBound(const K& key) const noexcept {
    BucketNode* x = Root();
    BucketNode* w = HeadNode();

    while (!x->m_isNull) {
        if (m_compare(*x->m_bucket.m_head[x->m_bucket.m_size - 1], key)) {
            x = x->m_right;    // descend right subtree
        } else { // x not less than key, remember it
            w = x;
            x = x->m_left;    // descend left subtree
        }
    }
    
    size_t offset = 0;
    if (!w->m_isNull) { // indication of end node
        auto it = std::lower_bound(w->m_bucket.m_head, w->m_bucket.m_head + w->m_bucket.m_size, key,
                                   [this](const Iter& first, const K& second) -> bool { return m_compare(*first, second); }
        );
        offset = it - w->m_bucket.m_head;
        assert(offset != w->m_bucket.m_size);
    }
    
    return iterator(w, offset);    // return best remembered candidate
}

template <uint32_t Capacity, typename Iter, typename Pred>
template <typename K>
typename OrderedMultiSet<Capacity, Iter, Pred>::iterator OrderedMultiSet<Capacity, Iter, Pred>::UpperBound(const K& key) const noexcept {
    BucketNode* x = Root();
    BucketNode* w = HeadNode();

    while (!x->m_isNull) {
        if (m_compare(key, x->m_bucket.m_head[x->m_bucket.m_size - 1])) {
            w = x;
            x = x->m_left;    // descend left subtree
        } else { // x not less than _Keyval, remember it
            x = x->m_right;    // descend right subtree
        }
    }
    
    size_t offset = 0;
    if (!w->m_isNull) { // indication of end node
        auto it = std::upper_bound(w->m_bucket.m_head, w->m_bucket.m_head + w->m_bucket.m_size, key,
                                   [this](const K& first, const Iter& second) -> bool { return m_compare(first, *second); }
                                   );

        offset = it - w->m_bucket.m_head;
        assert(offset != w->m_bucket.m_size);
    }
    
    return iterator(w, offset);    // return best remembered candidate
 }

// common equal range
template <uint32_t Capacity, typename Iter, typename Pred>
template<typename I, typename K>
std::pair<I, I> OrderedMultiSet<Capacity, Iter, Pred>::EqualRange(const K& key) const noexcept {
    BucketNode* x = Root();
    BucketNode* lowerNode = HeadNode();    // end() if search fails
    BucketNode* upperNode = HeadNode();    // end() if search fails

    while (!x->m_isNull) {
        if (m_compare(*x->m_bucket.m_head[x->m_bucket.m_size - 1], key)) {
            x = x->m_right;    // descend right subtree
        } else {    // x not less than key, remember it
            if (upperNode->m_isNull && m_compare(key, *x->m_bucket.m_head[x->m_bucket.m_size - 1])) {
                upperNode = x;    // x greater than key, remember it
            }
            lowerNode = x;
            x = x->m_left;    // descend left subtree
        }
    }
    x = upperNode->m_isNull ? Root() : upperNode->m_left;    // continue scan for upper bound
    while (!x->m_isNull) {
        if (m_compare(key, *x->m_bucket.m_head[x->m_bucket.m_size - 1])) {    // x greater than key, remember it
            upperNode = x;
            x = x->m_left;    // descend left subtree
        } else {
            x = x->m_right;    // descend right subtree
        }
    }
        
    size_t lowerOffset = 0;
    if (!lowerNode->m_isNull) { // indication of end node
        auto it = std::lower_bound(lowerNode->m_bucket.m_head, lowerNode->m_bucket.m_head + lowerNode->m_bucket.m_size, key,
                                   [this](const Iter& first, const K& second) -> bool { return m_compare(*first, second); }
                                   );

        lowerOffset = it - lowerNode->m_bucket.m_head;
        assert(lowerOffset != lowerNode->m_bucket.m_size);
    }
    
    size_t upperOffset = 0;
    if (!upperNode->m_isNull) { // indication of end node
        auto it = std::upper_bound(upperNode->m_bucket.m_head, upperNode->m_bucket.m_head + upperNode->m_bucket.m_size, key,
                                   [this](const K& first, const Iter& second) -> bool { return m_compare(first, *second); }
                                   );
        upperOffset = it - upperNode->m_bucket.m_head;
        assert(upperOffset != upperNode->m_bucket.m_size);
    }
    
    return {I(lowerNode, lowerOffset), I(upperNode, upperOffset)};
}

template <uint32_t Capacity, typename Iter, typename Pred>
template <typename I, typename K>
std::pair<I, I>
OrderedMultiSet<Capacity, Iter, Pred>::equal_range(const K& key) const noexcept {
    return EqualRange<I>(key);
}

template <uint32_t Capacity, typename Iter, typename Pred>
template <typename I, typename K>
I OrderedMultiSet<Capacity, Iter, Pred>::Find(const K& key) const noexcept {
    I it = LowerBound(key);
    if (it != end<const_iterator>() && !m_compare(key, **it)) {
        return it;
    }
    
    return end<const_iterator>();
}

template <uint32_t Capacity, typename Iter, typename Pred>
template <typename I, typename K>
I
OrderedMultiSet<Capacity, Iter, Pred>::find(const K& key) const noexcept {
    return Find<I>(key);
}

template <uint32_t Capacity, typename Iter, typename Pred>
std::pair<typename OrderedMultiSet<Capacity, Iter, Pred>::iterator, bool>
OrderedMultiSet<Capacity, Iter, Pred>::insert(const Iter& key) noexcept {
    // - Cases:
    // 1. Found a bucket where the new key can be inserted - no new bucket node or rebalnce is required
    // 2. Found a bucket where the new can supposed to be but bucket is full
    //      2.a - split the old bucket into two buckets and insert the new key into the correct position
    //      2.b - old bucket will surve as a parent node for the new node, reconnect the new node and do the rebalance
    // 3. First bucket - just create a new bucket node as a root and add the new key

    BucketNode* x = Root();
    BucketNode* w = HeadNode();
    bool addLeft = true;
    while (!x->m_isNull) {  // look for the bucket to insert
        w = x;
        if (m_compare(*key, *x->m_bucket.m_head[0])) { // i.e. 1 [2,3]
            x = x->m_left;
            addLeft = true;
        } else if (m_compare(*x->m_bucket.m_head[x->m_bucket.m_size - 1], *key)) { // [2,3] 4
            x = x->m_right;
            addLeft = false;
        } else { // i.e. 2 [1, 3]
            if (Capacity != 1) {
                break;
            }
            x = x->m_right;
            addLeft = false;
        }
    }

    if (!w->m_isNull) {
        if (w->m_bucket.m_size != Capacity) {
            assert(w->m_bucket.m_size > 0);
            size_t offset = w->m_bucket.m_size;
            auto* ptr = std::lower_bound(w->m_bucket.m_head, w->m_bucket.m_head + offset, key,
                                         [this](const Iter& first, const Iter& second) -> bool { return m_compare(*first, *second); }
                                         );
            
            if (ptr != w->m_bucket.m_head + offset) {
                // void* memmove( void* dest, const void* src, size_t count );
                offset = ptr - w->m_bucket.m_head;
                memmove(ptr + 1, ptr, sizeof(Iter) * (w->m_bucket.m_size - offset));
            }
            
            memcpy(ptr, &key, sizeof(key));
            ++w->m_bucket.m_size;
            ++m_totalItems;
            return {iterator(w, offset), true};
        } else if (w->m_bucket.m_size != 1) {
            assert(w->m_bucket.m_size == Capacity);
            // the bucket is full - split it
            size_t moffset = (Capacity - 1) / 2; // 2->0, 3->1, 4->1, 5->2, 6->2 ..., etc
            
            size_t offset = w->m_bucket.m_size;
            auto* ptr = std::lower_bound(w->m_bucket.m_head, w->m_bucket.m_head + offset, key,
                                         [this](const Iter& first, const Iter& second) -> bool { return m_compare(*first, *second); }
                                         );

            offset = ptr - w->m_bucket.m_head;
            x = allocateNode();

            if (offset <= moffset) { // copy the beginning
                if (offset != 0) {
                    // copy the first half of the bucket before offset, if any
                    memcpy(x->m_bucket.m_head, w->m_bucket.m_head, sizeof(Iter) * offset);
                }
                // new bucket size (without a new key)
                x->m_bucket.m_size = moffset + 1;
                // copy the rest of the first half with additional room at offset position for new key
                memcpy(x->m_bucket.m_head + offset + 1, w->m_bucket.m_head + offset, sizeof(Iter) * (x->m_bucket.m_size - offset));
                // adjust old bucket size
                w->m_bucket.m_size -= x->m_bucket.m_size;
                // move memory in the old bucket
                memmove(w->m_bucket.m_head, w->m_bucket.m_head + x->m_bucket.m_size, sizeof(Iter) * w->m_bucket.m_size);
                // assign key to the place it supposed to be
                memcpy(x->m_bucket.m_head + offset, &key, sizeof(key));
                ++x->m_bucket.m_size;
                
                if (w == LMost()) {
                    LMost() = x;
                }
                x->m_parent = w;
                x->m_left = w->m_left;
                if (!w->m_left->m_isNull) {
                    x->m_left = w->m_left;
                    w->m_left->m_parent = x;
                }
                w->m_left = x;
            } else {
                if (offset != moffset + 1) {
                    // copy the second half of the bucket before offset, if any
                    memcpy(x->m_bucket.m_head, w->m_bucket.m_head + moffset + 1, sizeof(Iter) * (offset - moffset - 1));
                }
                // new bucket size (without a new key)
                x->m_bucket.m_size = w->m_bucket.m_size - moffset - 1;
                // copy the rest of the second half with additional room at offset position for new key
                memcpy(x->m_bucket.m_head + (offset - moffset), w->m_bucket.m_head + offset,
                       sizeof(Iter) * (w->m_bucket.m_size - offset));
                w->m_bucket.m_size = moffset + 1;
                // assign key to the place it supposed to be
                memcpy(x->m_bucket.m_head + offset - moffset - 1, &key, sizeof(key));
                ++x->m_bucket.m_size;
                
                if (w == RMost()) {
                    RMost() = x;
                }
                x->m_parent = w;
                x->m_right = w->m_right;
                if (!w->m_right->m_isNull) {
                    x->m_right = w->m_right;
                    w->m_right->m_parent = x;
                }
                w->m_right = x;
            }
        } else {
            x = allocateNode();
            x->m_bucket.m_head[0] = key;
            x->m_bucket.m_size = 1;
            
            x->m_parent = w;
            if (addLeft) {
                w->m_left = x;
                if (w == LMost()) {
                    LMost() = x;
                }
            } else {
                w->m_right = x;
                if (w == RMost()) {
                    RMost() = x;
                }
            }
         }
    } else { // create Root and insert new key
        x = allocateNode();
        memcpy(x->m_bucket.m_head, &key, sizeof(key));
        x->m_bucket.m_size = 1;
        
        x->m_parent = w;
        Root() = x;
        LMost() = x;
        RMost() = x;
    }

    while (!x->m_parent->m_isBlack) {
        if (x->m_parent == x->m_parent->m_parent->m_left) {    // fixup red-red in left subtree
            w = x->m_parent->m_parent->m_right;
            if (!w->m_isBlack) {    // parent has two red children, blacken both
                x->m_parent->m_isBlack = true;
                w->m_isBlack = true;
                x->m_parent->m_parent->m_isBlack = false;
                x= x->m_parent->m_parent;
            } else {    // parent has red and black children
                if (x == x->m_parent->m_right) {    // rotate right child to left
                    x = x->m_parent;
                    LRotate(x);
                }
                x->m_parent->m_isBlack = true;
                x->m_parent->m_parent->m_isBlack = false;
                RRotate(x->m_parent->m_parent);
            }
        } else {    // fixup red-red in right subtree
            w = x->m_parent->m_parent->m_left;
            if (!w->m_isBlack) {    // parent has two red children, blacken both
                x->m_parent->m_isBlack = true;
                w->m_isBlack = true;
                x->m_parent->m_parent->m_isBlack = false;
                x = x->m_parent->m_parent;
            } else {    // parent has red and black children
                if (x == x->m_parent->m_left) {    // rotate left child to right
                    x = x->m_parent;
                    RRotate(x);
                }
                x->m_parent->m_isBlack = true;
                x->m_parent->m_parent->m_isBlack = false;
                LRotate(x->m_parent->m_parent);
            }
        }
    }
    Root()->m_isBlack = true;
    ++m_totalItems;
    return {iterator(x, 0), true};
}

template <uint32_t Capacity, typename Iter, typename Pred>
size_t OrderedMultiSet<Capacity, Iter, Pred>::erase(Iter key) noexcept {
    for (std::pair<iterator, iterator> p = equal_range<iterator>(*key); p.first != p.second; ++p.first) {
        if (*p.first == key) {
            BucketNode* node = p.first.GetNodePtr();
            size_t offset = p.first.GetOffset();
            if (offset + 1 == node->m_bucket.m_size) { // last item in the bucket
                if (1 == node->m_bucket.m_size) { // the only one item
                    Remove(node);
                    delete node;
                    --m_totalItems;
                    return 1;
                } else {
                    --node->m_bucket.m_size; // just reduce the size
                }
            } else {
                // void* memmove( void* dest, const void* src, size_t count );
                memmove(node->m_bucket.m_head + offset, node->m_bucket.m_head + offset + 1, sizeof(Iter) * (node->m_bucket.m_size - offset - 1));
                --node->m_bucket.m_size;
            }

            // check if the current semi-empty bucket if it's a leaf and can be merged with parent
            if (!node->m_parent->m_isNull // parent exists
                && node->m_bucket.m_size < Capacity / 2
                && node->m_parent->m_bucket.m_size < Capacity / 2) {
                bool isLeft = node == node->m_parent->m_left;
                
                if ((isLeft && node->m_right->m_isNull) || (!isLeft && node->m_left->m_isNull)) {
                    // if this node is left add to the head, if right one add to the tail
                    if (isLeft) {
                        memmove(node->m_parent->m_bucket.m_head + node->m_bucket.m_size, node->m_parent->m_bucket.m_head, sizeof(Iter) * node->m_parent->m_bucket.m_size);
                        memcpy(node->m_parent->m_bucket.m_head, node->m_bucket.m_head, sizeof(Iter) * node->m_bucket.m_size);
                        node->m_parent->m_bucket.m_size += node->m_bucket.m_size;
                    } else {
                        memcpy(node->m_parent->m_bucket.m_head + node->m_parent->m_bucket.m_size, node->m_bucket.m_head, sizeof(Iter) * node->m_bucket.m_size);
                        node->m_parent->m_bucket.m_size += node->m_bucket.m_size;
                    }
                    Remove(node);
                    delete node;
                }
            }
            --m_totalItems;
            return 1;
        }
    }
    
    return 0;
}

template <uint32_t Capacity, typename Iter, typename Pred>
void OrderedMultiSet<Capacity, Iter, Pred>::clear() noexcept {
    Destroy(Root());
    resetHead();
    m_totalItems = 0;
}

template <uint32_t Capacity, typename Iter, typename Pred>
void OrderedMultiSet<Capacity, Iter, Pred>::traverse() const noexcept {
    // find value by index
    for (auto bIt = begin(), eIt = end(); bIt != eIt; ++bIt) {
        printf("Item(ordered): %d\n", (*bIt)->i);
    }
    printf("_______________________\n");
}
