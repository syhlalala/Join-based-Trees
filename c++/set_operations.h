#pragma once

#include "pwbt.h"
#include "pavl.h"
#include "ptreap.h"
#include "prb.h"
#include "list_allocator.h"

#define NODE_LIMIT 150

template <typename T>
struct split_info {
    T* first;
    T* second;
    bool removed;
    typename T::value_type value;
    
    split_info(T* _first, T* _second, bool _removed) {
        first = _first;
        second = _second;
        removed = _removed;
    }
    
    split_info() {};
};
    
template <typename T, typename K> 
split_info<T> t_split(T* bst, K e) {
    if (!bst) return split_info<T>(NULL, NULL, false);
    
    T* join_node = bst;
    T* lsub = bst->lc, *rsub = bst->rc;
    
    const K key = bst->get_key();
    const typename T::value_type value = bst->get_value();
    
    if (bst->ref_cnt >= 2) { 
        join_node = bst->copy();
        decrease_recursive(bst);
    }
    
    if (e == key) {
        split_info<T> ret(lsub, rsub, true);
        ret.value = value;
        decrease(join_node);
        return ret;
    }

    if (e > key) {
        split_info<T> bstpair = t_split(rsub, e);
        bstpair.first = t_join(lsub, bstpair.first, join_node);
        return bstpair;
    }
    else {
        split_info<T> bstpair = t_split(lsub, e);
        bstpair.second = t_join(bstpair.second, rsub, join_node);
        return bstpair;
    }
}

template <typename T>
T* t_join2(T* b1, T* b2) {
    if (!b1) return b2;
    if (!b2) return b1;
    
    if (compareValue(b1) > compareValue(b2)) {
        T* join_node = b1;
        if (b1->ref_cnt >= 2) {
            join_node = b1->copy();
            decrease_recursive(b1);
        }

        return t_join(join_node->lc, t_join2(join_node->rc, b2), join_node);
     } 
     else {
        T* join_node = b2;
        if (b2->ref_cnt >= 2) {
            join_node = b2->copy();
            decrease_recursive(b2);
        }

        return t_join(t_join2(b1, join_node->lc), join_node->rc, join_node);
    }
}

template <typename T, typename BinaryOp> 
T* t_union(T* b1, T* b2, BinaryOp op) {
    if (!b1) return b2;
    if (!b2) return b1;
    
    split_info<T> bsts = t_split(b1, b2->get_key());
    
    T* join_node = b2;
    T* lsub = b2->lc, *rsub = b2->rc;
    
    if (b2->ref_cnt >= 2) {
        join_node = b2->copy();
        decrease_recursive(b2);
    }
    
    T* leftTree, *rightTree;
    size_t mn = getNodeCount(lsub);

    if (mn >= NODE_LIMIT) {
        leftTree = cilk_spawn t_union(bsts.first, lsub, op);
        rightTree = t_union(bsts.second, rsub, op);
        cilk_sync;      
    } else {
        leftTree = t_union(bsts.first, lsub, op);
        rightTree = t_union(bsts.second, rsub, op);
    }
    
    if (bsts.removed)
        join_node->set_value(op(bsts.value, join_node->get_value()));
    
    return t_join(leftTree, rightTree, join_node);
}


template <typename T, typename BinaryOp>
T* t_intersect(T* b1, T* b2, BinaryOp op) {
    if (!b1) { 
        decrease_recursive(b2); 
        return NULL; 
    }
    if (!b2) {
        decrease_recursive(b1);
        return NULL;
    }

    split_info<T> bsts = t_split(b1, b2->get_key());
    
    T* join_node = b2;
    T* lsub = b2->lc, *rsub = b2->rc;
    
    if (b2->ref_cnt >= 2) {
        join_node = b2->copy();
        decrease_recursive(b2);
    }
    
    T* leftTree, *rightTree;
    size_t mn = getNodeCount(lsub);
    
    if (mn >= NODE_LIMIT) {
        leftTree = cilk_spawn t_intersect(bsts.first, lsub, op);
        rightTree = t_intersect(bsts.second, rsub, op);
        cilk_sync;
    } else {
        leftTree = t_intersect(bsts.first, lsub, op);
        rightTree = t_intersect(bsts.second, rsub, op);
    }       
    
    if (bsts.removed) {
        join_node->set_value(op(bsts.value, join_node->get_value()));
        return t_join(leftTree, rightTree, join_node);
    } else {
        decrease(join_node);
        return t_join2(leftTree, rightTree);    
    }
}


template <typename T>
T* t_difference(T* b1, T* b2) {
    if (!b1) {
        decrease_recursive(b2);
        return NULL;
    }
    if (!b2) return b1;
    
    split_info<T> bsts = t_split(b2, b1->get_key());

    T* join_node = b1;
    T* lsub = b1->lc, *rsub = b1->rc;
    
    if (b1->ref_cnt >= 2) {
        join_node = b1->copy();
        decrease_recursive(b1);
    }

    T* leftTree, *rightTree;
    size_t mn = getNodeCount(lsub);
    
    if (mn >= NODE_LIMIT) {
        leftTree = cilk_spawn t_difference(lsub, bsts.first);
        rightTree = t_difference(rsub, bsts.second);
        cilk_sync;  
    } else {
        leftTree = t_difference(lsub, bsts.first);
        rightTree = t_difference(rsub, bsts.second);
    }       
    
    if (bsts.removed) {
        decrease(join_node);
        return t_join2(leftTree, rightTree);
    } else {
        return t_join(leftTree, rightTree, join_node);
    }   
}

template<typename T, typename Func>
T* t_filter(T* b, const Func& f) {
    if (!b) return NULL;

    T* join_node = b;
    T* lsub = b->lc, *rsub = b->rc;

    if (b->ref_cnt >= 2) {
        join_node = b->copy();
        decrease_recursive(b);
    }

    T* leftTree, *rightTree;
    size_t mn = getNodeCount(lsub);
    
    if (mn >= NODE_LIMIT) {
        leftTree = cilk_spawn t_filter(lsub, f);
        rightTree = t_filter(rsub, f);
        cilk_sync;  
    } else {
        leftTree = t_filter(lsub, f);
        rightTree = t_filter(rsub, f);
    }

    if ( f(b->get_entry()) ) {
        return t_join(leftTree, rightTree, join_node);
    } else {
        decrease(join_node);
        return t_join2(leftTree, rightTree);
    }
}

template<typename NodeType, typename T, typename Func>
void t_forall(T* b, const Func& f, NodeType*& joinNode) {
    if (!b) {
        joinNode = NULL;
        return;
    }

    typename NodeType::entry_type entry = make_pair(b->get_key(), f(b->get_entry()));

    joinNode = new NodeType(entry);

    cilk_spawn t_forall(b->lc, f, joinNode->lc);
    t_forall<NodeType>(b->rc, f, joinNode->rc);
    cilk_sync;

    joinNode->update();
}
