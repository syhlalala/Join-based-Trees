#pragma once

#include "list_allocator.h"
#include "utils.h"

#define NODE_LIMIT 150

template<typename V>
struct get_left {
    V operator () (const V& a, const V& b) { return a; }
};

template <typename T> T* rebalance(T* current);
template <typename T> bool isBalanced(T* current);

template <typename T> T* persist_rotateRight(T* t) {
    T* root = t->lc;
    T* rsub = root->rc;
    root->rc = t, t->lc = rsub;
    root->rc->update();
    root->update();
    return root;
}

template <typename T> T* persist_rotateLeft(T* t) {
    T* root = t->rc; 
    T* lsub = root->lc;
    root->lc = t, t->rc = lsub;
    root->lc->update();
    root->update();
    return root;
}

template <typename T> T* persist_doubleRotateRight(T* t) {
    if (t->lc->rc && t->lc->rc->ref_cnt > 1) {
       T* tmp = t->lc->rc;
       t->lc->rc = t->lc->rc->copy();
       decrease_recursive(tmp);
    }
    
    t->lc = persist_rotateLeft(t->lc);
    return persist_rotateRight(t);
}


template <typename T> T* persist_doubleRotateLeft(T* t) {
    if (t->rc->lc && t->rc->lc->ref_cnt > 1) {
       T* tmp = t->rc->lc;
       t->rc->lc = t->rc->lc->copy();
       decrease_recursive(tmp);
    }
        
    t->rc = persist_rotateRight(t->rc);
    return persist_rotateLeft(t);
}

template <typename T> size_t get_height(T* t) {
    if (!t) return 0;
    return 1 + max(get_height(t->lc), get_height(t->rc));
}

template <typename T> bool checkBalance(T* t) {
    if(!t) return 1;
    bool ret = isBalanced(t);
    ret &= checkBalance(t->rc);
    ret &= checkBalance(t->lc);
    return ret;
}

template <typename T> bool checkBST(T* t) {
    if (!t) return 1;
    bool ret = 1;
    if (t->rc) ret &= t->key <= t->rc->key;
    if (t->lc) ret &= t->key >= t->lc->key;

    ret &= checkBST(t->lc);
    ret &= checkBST(t->rc);
    return ret;
}

template <typename T> inline size_t getNodeCount(T* t) {
    return !t ? 0 : t->node_cnt;
}

template <typename T> bool decrease(T* t) {
    if (t) { 
        utils::writeAdd(&t->ref_cnt, -1);
        if (!t->ref_cnt) {
            t->collect();
            return true;
        }
    }
    return false;
}

template <typename T> inline void increase(T* t) {
    if (t) utils::writeAdd(&t->ref_cnt, 1);
}

template <typename T> void decrease_recursive(T* t) {
    if (!t) return;

    T* lsub = t->lc;
    T* rsub = t->rc;

    if (decrease(t)) {
        size_t sz = getNodeCount(lsub);
        if (sz >= NODE_LIMIT) {
            cilk_spawn decrease_recursive(lsub);
            decrease_recursive(rsub);
            cilk_sync;
        } else {
            decrease_recursive(lsub);
            decrease_recursive(rsub);
       }
    }
}

