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
typename OrderedMultiSet<Capacity, Iter, Pred>::BucketNode* OrderedMultiSet<Capacity, Iter, Pred>::HeadNode() const noexcept {
    return const_cast<BucketNode*>(&m_headNode);
}

template <uint32_t Capacity, typename Iter, typename Pred>
typename OrderedMultiSet<Capacity, Iter, Pred>::BucketNode*& OrderedMultiSet<Capacity, Iter, Pred>::Root() const noexcept {
    return const_cast<BucketNode*&>(m_headNode.m_parent);
}

template <uint32_t Capacity, typename Iter, typename Pred>
typename OrderedMultiSet<Capacity, Iter, Pred>::BucketNode*& OrderedMultiSet<Capacity, Iter, Pred>::RMost() noexcept { // return rightmost node in non-mutable tree
    return m_headNode.m_right;
}

template <uint32_t Capacity, typename Iter, typename Pred>
typename OrderedMultiSet<Capacity, Iter, Pred>::BucketNode*& OrderedMultiSet<Capacity, Iter, Pred>::LMost() const noexcept { // return leftmost node in non-mutable tree
    return const_cast<BucketNode*&>(m_headNode.m_left);
}

template <uint32_t Capacity, typename Iter, typename Pred>
void OrderedMultiSet<Capacity, Iter, Pred>::LRotate(BucketNode* w) noexcept {
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
void OrderedMultiSet<Capacity, Iter, Pred>::RRotate(BucketNode* w) noexcept {
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
    BucketNode* r;        // the node to recolor as needed
    BucketNode* rParent;  // parent of r (which may be nil)
    BucketNode* x = z;
    
    if (x->m_left->m_isNull) {
        r = x->m_right;    // stitch up right subtree
    } else if (x->m_right->m_isNull) {
        r = x->m_left;    // stitch up left subtree
    } else {    // two subtrees, must lift successor node to replace erased
        iterator next(z, z->m_bucket.m_size - 1);
        ++next;
        x = next.GetNodePtr();    // x is successor node
        r = x->m_right;    // r is only subtree
    }
    
    if (x == z) {    // at most one subtree, relink it
        rParent = z->m_parent;
        if (!r->m_isNull) {
            r->m_parent = rParent;    // link up
        }
        
        if (Root() == z) {
            Root() = r;    // link down from root
        } else if (rParent->m_left == z) {
            rParent->m_left = r;    // link down to left
        } else {
            rParent->m_right = r;    // link down to right
        }
        
        if (LMost() == z) {
            LMost() = r->m_isNull
            ? rParent    // smallest is parent of erased node
            : Min(r);    // smallest in relinked subtree
        }
        
        if (RMost() == z) {
            RMost() = r->m_isNull
            ? rParent    // largest is parent of erased node
            : Max(r);    // largest in relinked subtree
        }
    } else {    // erased has two subtrees, x is successor to erased
        z->m_left->m_parent = x; // link left up
        x->m_left = z->m_left; // link successor down

        if (x == z->m_right) {
            rParent = x;    // successor is next to erased
        } else {    // successor further down, link in place of erased
            rParent = x->m_parent;
            if (!r->m_isNull) {
                r->m_parent = rParent;    // link fix up
            }
            rParent->m_left = r;    // link fix down
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
        for (; r != Root() && r->m_isBlack; rParent = r->m_parent) {
            if (r == rParent->m_left) {    // fixup left subtree
                x = rParent->m_right;
                if (!x->m_isBlack) {    // rotate red up from right subtree
                    x->m_isBlack = true;
                    rParent->m_isBlack = false;
                    LRotate(rParent);
                    x = rParent->m_right;
                }
                
                if (x->m_isNull) {
                    r = rParent;    // shouldn't happen
                } else if (x->m_left->m_isBlack && x->m_right->m_isBlack) {    // redden right subtree with black children
                    x->m_isBlack = false;
                    r = rParent;
                } else {    // must rearrange right subtree
                    if (x->m_right->m_isBlack) {    // rotate red up from left sub-subtree
                        x->m_left->m_isBlack = true;
                        x->m_isBlack = false;
                        RRotate(x);
                        x= rParent->m_right;
                    }
                    
                    x->m_isBlack = rParent->m_isBlack;
                    rParent->m_isBlack = true;
                    x->m_right->m_isBlack = true;
                    LRotate(rParent);
                    break;    // tree now recolored/rebalanced
                }
            } else {    // fixup right subtree
                x = rParent->m_left;
                if (!x->m_isBlack) {    // rotate red up from left subtree
                    x->m_isBlack = true;
                    rParent->m_isBlack = false;
                    RRotate(rParent);
                    x = rParent->m_left;
                }
                if (x->m_isNull) {
                    r = rParent;    // shouldn't happen
                } else if (x->m_right->m_isBlack && x->m_left->m_isBlack) {    // redden left subtree with black children
                    x->m_isBlack = false;
                    r = rParent;
                } else {    // must rearrange left subtree
                    if (x->m_left->m_isBlack) {    // rotate red up from right sub-subtree
                        x->m_right->m_isBlack = true;
                        x->m_isBlack = false;
                        LRotate(x);
                        x = rParent->m_left;
                    }
                    
                    x->m_isBlack = rParent->m_isBlack;
                    rParent->m_isBlack = true;
                    x->m_left->m_isBlack = true;
                    RRotate(rParent);
                    break;    // tree now recolored/rebalanced
                }
            }
        }
        r->m_isBlack = true;
    }
}

template <uint32_t Capacity, typename Iter, typename Pred>
typename OrderedMultiSet<Capacity, Iter, Pred>::BucketNode* OrderedMultiSet<Capacity, Iter, Pred>::allocateNode() {
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
/*static*/
void OrderedMultiSet<Capacity, Iter, Pred>::Destroy(BucketNode* node) noexcept {
    // recursive calls to the depth of the tree, with a standard stack size it could accomodate up to 64K recursions.
    // i.e. 2^64K nodes - unrealistic.
    if (!node->m_isNull) {
        Destroy(node->m_left);
        Destroy(node->m_right);
        delete node;
    }
}

template <uint32_t Capacity, typename Iter, typename Pred>
template <typename K>
std::pair<typename OrderedMultiSet<Capacity, Iter, Pred>::iterator, typename OrderedMultiSet<Capacity, Iter, Pred>::iterator>
OrderedMultiSet<Capacity, Iter, Pred>::equal_range(const K& key) const noexcept {
    const BucketNode* x = Root();
    const BucketNode* l = HeadNode();    // end() if search fails
    const BucketNode* u = HeadNode();    // end() if search fails

    while (!x->m_isNull) {
        if (m_compare(*x->m_bucket.m_head[x->m_bucket.m_size - 1], key)) {
            x = x->m_right;    // descend right subtree
        } else {    // x not less than key, remember it
            if (u->m_isNull && m_compare(key, *x->m_bucket.m_head[x->m_bucket.m_size - 1])) {
                u = x;    // x greater than key, remember it
            }
            l = x;
            x = x->m_left;    // descend left subtree
        }
    }
    x = u->m_isNull ? Root() : u->m_left;    // continue scan for upper bound
    while (!x->m_isNull) {
        if (m_compare(key, *x->m_bucket.m_head[x->m_bucket.m_size - 1])) {    // x greater than key, remember it
            u = x;
            x = x->m_left;    // descend left subtree
        } else {
            x = x->m_right;    // descend right subtree
        }
    }
        
    size_t lOffset = 0;
    if (!l->m_isNull) { // indication of not end node, head is valid
        lOffset = std::lower_bound(l->m_bucket.m_head, l->m_bucket.m_head + l->m_bucket.m_size, key,
                                   [this](const Iter& first, const K& second) -> bool { return m_compare(*first, second); }
        ) - l->m_bucket.m_head;
        assert(lOffset != l->m_bucket.m_size);
    }
    
    size_t uOffset = 0;
    if (!u->m_isNull) { // indication of end node
        uOffset = std::upper_bound(u->m_bucket.m_head, u->m_bucket.m_head + u->m_bucket.m_size, key,
                                   [this](const K& first, const Iter& second) -> bool { return m_compare(first, *second); }
        ) - u->m_bucket.m_head;
        assert(uOffset != u->m_bucket.m_size);
    }
    
    return {iterator(l, lOffset), iterator(u, uOffset)};
}

template <uint32_t Capacity, typename Iter, typename Pred>
template <typename K>
bool OrderedMultiSet<Capacity, Iter, Pred>::is_equal(const K& first, const K& second) const noexcept {
    return !m_compare(first, second) && !m_compare(second, first);
}

template <uint32_t Capacity, typename Iter, typename Pred>
template <typename K>
typename OrderedMultiSet<Capacity, Iter, Pred>::iterator
OrderedMultiSet<Capacity, Iter, Pred>::find(const K& key) const noexcept {
    const BucketNode* x = Root();
    const BucketNode* l = HeadNode();

    while (!x->m_isNull) {
        if (m_compare(*x->m_bucket.m_head[x->m_bucket.m_size - 1], key)) {
            x = x->m_right;    // descend right subtree
        } else { // x not less than key, remember it
            l = x;
            x = x->m_left;    // descend left subtree
        }
    }
    
    size_t offset = 0;
    if (!l->m_isNull) { // indication of end node
        offset = std::lower_bound(l->m_bucket.m_head, l->m_bucket.m_head + l->m_bucket.m_size, key,
                                   [this](const Iter& first, const K& second) -> bool { return m_compare(*first, second); }
        ) - l->m_bucket.m_head;
        assert(offset != l->m_bucket.m_size);
    }
    
    iterator lower(l, offset);
    if (lower != end() && !m_compare(key, **lower)) {
        return lower;
    }
    
    return end();
}

template <uint32_t Capacity, typename Iter, typename Pred>
bool
OrderedMultiSet<Capacity, Iter, Pred>::insert(bool, const Iter& key) noexcept {
    // - Cases:
    // 1. Found a bucket where the new key can be inserted - no new bucket node or rebalance is required
    // 2. Found a bucket where the new key is supposed to be but bucket is full
    //      2.a - split the old bucket into two buckets and insert the new key into the correct position
    //      2.b - old bucket will serve as a parent node for the new node, reconnect the new node and do the rebalance
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
            return true;
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
    return true;
}

template <uint32_t Capacity, typename Iter, typename Pred>
size_t OrderedMultiSet<Capacity, Iter, Pred>::erase(Iter key) noexcept {
    for (auto p = equal_range(*key); p.first != p.second; ++p.first) {
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
    // direct
    for (auto bDirIt = begin(), eDirIt = end(); bDirIt != eDirIt; ++bDirIt) {
        printf("Item(ordered): %d\n", (*bDirIt)->i);
    }
    printf("_______________________\n");
}
