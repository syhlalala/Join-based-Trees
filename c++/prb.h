#include "abstract_node.h"
#include "commons.h"
#include "list_allocator.h"

typedef enum { RED, BLACK, UNDEFCOLOR } Color;

template<typename K, typename V>
class RBNode : public AbstractNode<K, V> {
public:
    typedef typename AbstractNode<K, V>::entry_type     entry_type;
    typedef RBNode<K, V>                                node_type;

    size_t heightOrSize;
    Color color;
    
    node_type* lc;
    node_type* rc;

    RBNode() {}
    RBNode(entry_type, RBNode* left = NULL, RBNode* right = NULL);
    
    static void* operator new(size_t size) {
        return node_allocator<node_type>::talloc();
    }   
    
    virtual node_type* copy();
    virtual inline void update();
    virtual inline void collect();
};


template<typename K, typename V, typename AugmOp>
class AugmentedRBNode : public RBNode<K, V> {
public:
    typedef typename AbstractNode<K, V>::entry_type     entry_type;
    typedef AugmentedRBNode<K, V, AugmOp>               node_type;

    AugmOp reduce_operator;
    V accumulation;

    AugmentedRBNode(entry_type, RBNode<K, V>* lc = NULL, RBNode<K, V>* rc = NULL);
    
    static void* operator new(size_t size) {
        return node_allocator<node_type>::talloc();
    }
    
    RBNode<K, V>* copy();
    inline void update();
    inline void collect();
};


template<typename K, typename V>
inline void RBNode<K, V>::collect() {
    this->entry.~entry_type();
    node_allocator<node_type>::collect(this);
}

template<typename K, typename V, typename AugmOp>
inline void AugmentedRBNode<K, V, AugmOp>::collect() {
    this->entry.~entry_type();
    node_allocator<node_type>::collect(this);
}

template<typename K, typename V>
RBNode<K, V>* RBNode<K, V>::copy() {
    RBNode* ret = new node_type(this->entry, lc, rc);
    ret->color = color;
    ret->heightOrSize = heightOrSize;
    increase(lc);
    increase(rc);
    
    return ret;
}

template<typename K, typename V, typename AugmOp>
RBNode<K, V>* AugmentedRBNode<K, V, AugmOp>::copy() {
    RBNode<K, V>* ret = new node_type(this->entry, this->lc, this->rc);
    ret->color = this->color;
    ret->heightOrSize = this->heightOrSize;
    increase(this->lc);
    increase(this->rc);
    
    return ret;
}

template<typename K, typename V>
inline size_t getHeightOrSize(RBNode<K, V>* t) {
    return t ? t->heightOrSize : 1;
}

template<typename K, typename V>
inline size_t compareValue(RBNode<K, V>* t) {
    return getHeightOrSize(t);
}

template<typename K, typename V>
inline Color getColor(RBNode<K, V>* t) {
    return t ? t->color : BLACK;
}

template<typename K, typename V>
bool isBalanced(RBNode<K, V>* t) {
    return !t || getHeightOrSize(t->lc) == getHeightOrSize(t->rc);
}

template<typename K, typename V>
inline void RBNode<K, V>::update() {
    heightOrSize = max(getHeightOrSize(lc), getHeightOrSize(rc)) + (color == BLACK);
    this->node_cnt = 1 + getNodeCount(lc) + getNodeCount(rc);
}

template<typename K, typename V, typename AugmOp>
inline void AugmentedRBNode<K, V, AugmOp>::update() {
    RBNode<K, V>::update();
    
    accumulation = this->value;
    if (this->lc) accumulation = reduce_operator(accumulation, ((node_type*)this->lc)->accumulation);
    if (this->rc) accumulation = reduce_operator(accumulation, ((node_type*)this->rc)->accumulation);
}

template<typename K, typename V>
RBNode<K, V>::RBNode(entry_type entry, RBNode* left, RBNode* right) {
    this->ref_cnt = 1;
    this->entry = entry;
    lc = left, rc = right, color = BLACK;
    update();
}

template<typename K, typename V, typename AugmOp>
AugmentedRBNode<K, V, AugmOp>::AugmentedRBNode(entry_type entry, RBNode<K, V>* left, RBNode<K, V>* right)
    : RBNode<K, V>(entry, left, right) {
    reduce_operator = AugmOp();
    update();
}

template<typename K, typename V>
RBNode<K, V>* p_rebalanceLeft(RBNode<K, V>* t) {
    if (getColor(t) == RED && getColor(t->lc) == RED) {
        t->color = BLACK;
        t->heightOrSize++;
        return t;
    }
    
    if (getColor(t) == BLACK && getColor(t->lc) == BLACK
    && (getHeightOrSize(t) == getHeightOrSize(t->lc))) {
        RBNode<K, V>* tmp = persist_rotateRight(t);
        tmp->color = RED;
        tmp->lc->color = BLACK;
        tmp->lc->heightOrSize++;
        tmp->heightOrSize = tmp->lc->heightOrSize;
        return tmp;
    }
    
    return t;
}

template<typename K, typename V>
RBNode<K, V>* p_rebalanceRight(RBNode<K, V>* t) {
    if (getColor(t) == RED && getColor(t->rc) == RED) {
        t->color = BLACK;
        t->heightOrSize++;
        return t;
    }
    
    if (getColor(t) == BLACK && getColor(t->rc) == BLACK
    && (getHeightOrSize(t) == getHeightOrSize(t->rc))) {
        RBNode<K, V>* tmp = persist_rotateLeft(t);
        tmp->color = RED;
        tmp->rc->color = BLACK;
        tmp->rc->heightOrSize++;
        tmp->heightOrSize = tmp->rc->heightOrSize;
        return tmp;
    }
    
    return t;
}

template<typename K, typename V>
RBNode<K, V>* p_joint_right(RBNode<K, V>* t1, RBNode<K, V>* t2, RBNode<K, V>* k) { 
    RBNode<K, V>* tmp = NULL;
    if (getHeightOrSize(t1) == getHeightOrSize(t2) && getColor(t1) == BLACK) {
        k->color = RED;
        k->lc = t1, k->rc = t2;
        k->update();
        return k;
    } 
    
    if (t1->ref_cnt >= 2) {
        tmp = t1->copy();
        tmp->rc = p_joint_right(t1->rc, t2, k);
        decrease_recursive(t1);
    } else {
        tmp = t1;
        tmp->rc = p_joint_right(t1->rc, t2, k);
    }
    
    RBNode<K, V>* ret = p_rebalanceRight(tmp);
    ret->update();
    return ret;
}

template<typename K, typename V>
RBNode<K, V>* p_joint_left(RBNode<K, V>* t1, RBNode<K, V>* t2, RBNode<K, V>* k) {
    RBNode<K, V>* tmp = NULL;
    
    if (getHeightOrSize(t1) == getHeightOrSize(t2) && getColor(t2) == BLACK) {
        k->color = RED;
        k->lc = t1, k->rc = t2;
        k->update();
        return k;
    } 

    if (t2->ref_cnt >= 2) {
        tmp = t2->copy();
        tmp->lc = p_joint_left(t1, t2->lc, k);
        decrease_recursive(t2);
    } else {
        tmp = t2;
        tmp->lc = p_joint_left(t1, t2->lc, k);
    }
    
    RBNode<K, V>* ret = p_rebalanceLeft(tmp);
    ret->update();
    return ret;
}


template<typename K, typename V>
RBNode<K, V>* t_join(RBNode<K, V>* t1, RBNode<K, V>* t2, RBNode<K, V>* k) {
    RBNode<K, V>* tmp1 = NULL, *tmp2 = NULL;

    bool copy1 = 0;
    bool copy2 = 0; 

    if (t1) {
        if (t1->ref_cnt >= 2) { tmp1 = t1->copy(); copy1 = 1; }
        else tmp1 = t1;
        
        if (t1->color == RED) tmp1->heightOrSize++;
        tmp1->color = BLACK;
    }
    if (t2) {
        if (t2->ref_cnt >= 2) { tmp2 = t2->copy(); copy2 = 1; }
        else tmp2 = t2;
        
        if (t2->color == RED) tmp2->heightOrSize++;
        tmp2->color = BLACK;
     }
     
    RBNode<K, V>* ret;
    if (getHeightOrSize(tmp2) < getHeightOrSize(tmp1))
        ret = p_joint_right(tmp1, tmp2, k);
    else if (getHeightOrSize(tmp2) > getHeightOrSize(tmp1))
        ret = p_joint_left(tmp1, tmp2, k);
    else {
        ret = k;
        ret->color = BLACK;
        ret->lc = tmp1, ret->rc = tmp2;
        ret->update();
    }
    
    if (copy1) decrease_recursive(t1);
    if (copy2) decrease_recursive(t2);
    return ret;
}

template<typename K, typename V>
RBNode<K, V>* t_insert(RBNode<K, V>* t, const pair<K, V>& elem) {}
