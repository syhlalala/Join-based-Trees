#include "abstract_node.h"
#include "commons.h"
#include "list_allocator.h"

const double alpha = 0.29;
const double beta  = (1-2*alpha) / (1-alpha);

template<typename K, typename V>
class WBNode : public AbstractNode<K, V> {
public:
    typedef typename AbstractNode<K, V>::entry_type     entry_type;
    typedef WBNode<K, V>                                node_type;

    size_t heightOrSize;

    node_type* lc;
    node_type* rc;

    WBNode(){};
    WBNode(entry_type, WBNode* lc = NULL, WBNode* rc = NULL);

    static void* operator new(size_t size) {
        return node_allocator<node_type>::talloc();
    }
    
    virtual node_type* copy();
    virtual inline void update();
    virtual inline void collect();
};


template<typename K, typename V, typename AugmOp>
class AugmentedWBNode : public WBNode<K, V> {
public:
    typedef typename AbstractNode<K, V>::entry_type     entry_type;
    typedef AugmentedWBNode<K, V, AugmOp>               node_type;

    AugmOp reduce_operator;
    V accumulation;

    AugmentedWBNode(entry_type, WBNode<K, V>* lc = NULL, WBNode<K, V>* rc = NULL);
    
    static void* operator new(size_t size) {
        return node_allocator<node_type>::talloc();
    }
    
    inline WBNode<K, V>* copy();
    inline void update();
    inline void collect();
};


template<typename K, typename V>
inline void WBNode<K, V>::collect() {
    this->entry.~entry_type();
    node_allocator<WBNode<K, V> >::collect(this);
}

template<typename K, typename V, typename AugmOp>
inline void AugmentedWBNode<K, V, AugmOp>::collect() {
    this->entry.~entry_type();
    node_allocator<node_type>::collect(this);
}

template<typename K, typename V>
inline size_t getHeightOrSize(WBNode<K, V>* t) {
    return !t ? 1 : t->heightOrSize;
}

template<typename K, typename V>
inline size_t compareValue(WBNode<K, V>* t) {
    return getHeightOrSize(t);
}

template<typename K, typename V>
inline void WBNode<K, V>::update() {
    heightOrSize = getHeightOrSize(lc) + getHeightOrSize(rc);
    this->node_cnt = 1 + getNodeCount(lc) + getNodeCount(rc);
}

template<typename K, typename V, typename AugmOp>
inline void AugmentedWBNode<K, V, AugmOp>::update() {
    WBNode<K, V>::update();
    accumulation = this->value;
    if (this->lc) accumulation = reduce_operator(accumulation, ((node_type*)this->lc)->accumulation);
    if (this->rc) accumulation = reduce_operator(accumulation, ((node_type*)this->rc)->accumulation);
}

template<typename K, typename V>
inline WBNode<K, V>* WBNode<K, V>::copy() {
    WBNode* ret = new node_type(this->entry, lc, rc);
    increase(lc);
    increase(rc);
    return ret;
}

template<typename K, typename V, typename AugmOp>
inline WBNode<K, V>* AugmentedWBNode<K, V, AugmOp>::copy() {
    WBNode<K, V>* ret = new node_type(this->entry, this->lc, this->rc);
    increase(this->lc);
    increase(this->rc);
    return ret; 
}

template<typename K, typename V>
WBNode<K, V>::WBNode(entry_type entry, WBNode* l, WBNode* r) {
    this->entry = entry;
    this->ref_cnt = 1;
    lc = l, rc  = r;
    update();
}

template<typename K, typename V, typename AugmOp>
AugmentedWBNode<K, V, AugmOp>::AugmentedWBNode(entry_type entry, WBNode<K, V>* lc, WBNode<K, V>* rc) 
    : WBNode<K, V>(entry, lc, rc) {
    reduce_operator = AugmOp();
    update();
}

template<typename K, typename V>
inline bool isTooHeavy(WBNode<K, V>* l, WBNode<K, V>* r) {
    return alpha * getHeightOrSize(l) > (1 - alpha) * getHeightOrSize(r);
}

template<typename K, typename V>
inline bool isBalanced(WBNode<K, V>* t) {
    return !t || (!isTooHeavy(t->lc, t->rc) && !isTooHeavy(t->rc, t->lc));
}

template<typename K, typename V>
bool is_single_rotation(WBNode<K, V>* t, bool child) {
    int balance = getHeightOrSize(t->lc);
    if (child)
        balance = getHeightOrSize(t) - balance;

    return balance <= beta*getHeightOrSize(t);
}

template<typename K, typename V>
WBNode<K, V>* persist_rebalanceSingle(WBNode<K, V>* t) {
    WBNode<K, V>* ret = t;
    if (isTooHeavy(t->lc, t->rc)) {   
        if (is_single_rotation(t->lc, 1)) ret = persist_rotateRight(t);
        else ret = persist_doubleRotateRight(t);
    
    }else if(isTooHeavy(t->rc, t->lc)) {
        if (is_single_rotation(t->rc, 0)) ret = persist_rotateLeft(t);
        else ret = persist_doubleRotateLeft(t);
    }

    return ret;
}

template<typename K, typename V>
WBNode<K, V>* t_join(WBNode<K, V>* t1, WBNode<K, V>* t2, WBNode<K, V>* k) {
    WBNode<K, V>* tmp = NULL;
    
    if (isTooHeavy(t1, t2)) {
        if (t1->ref_cnt >= 2) {
            tmp = t1->copy();
            tmp->rc = t_join(t1->rc, t2, k);
            decrease_recursive(t1);   
        } else {
            tmp = t1;
            tmp->rc = t_join(t1->rc, t2, k);
        }
    } else if (isTooHeavy(t2, t1)) {
        if (t2->ref_cnt >= 2) {
            tmp = t2->copy();
            tmp->lc = t_join(t1, t2->lc, k);
            decrease_recursive(t2);
        } else {
            tmp = t2;
            tmp->lc = t_join(t1, t2->lc, k);
        }
    } else {
        k->lc = t1, k->rc = t2;
        k->update();
        return k;
    }
    
    tmp->update();
    return persist_rebalanceSingle(tmp);
}

template<typename K, typename V>
WBNode<K, V>* t_insert(WBNode<K, V>* t, const pair<K, V>& elem) {}
