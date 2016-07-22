#include "abstract_node.h"
#include "commons.h"
#include "list_allocator.h"

template<typename K, typename V>
class AVLNode : public AbstractNode<K, V> {
public:
    typedef typename AbstractNode<K, V>::entry_type     entry_type;
    typedef AVLNode<K, V>                               node_type;

    size_t heightOrSize;

    node_type* lc;
    node_type* rc;

    AVLNode() {}
    AVLNode(entry_type, AVLNode* left = NULL, AVLNode* right = NULL);

    static void* operator new(size_t size) {
        return node_allocator<node_type>::talloc();
    }

    virtual node_type* copy();
    virtual inline void update();
    virtual inline void collect();
};


template<typename K, typename V, typename AugmOp>
class AugmentedAVLNode : public AVLNode<K, V> {
public:
    typedef typename AbstractNode<K, V>::entry_type entry_type;
    typedef AugmentedAVLNode<K, V, AugmOp> node_type;

    AugmOp reduce_operator;
    V accumulation;

    AugmentedAVLNode(entry_type, AVLNode<K, V>* lc = NULL, AVLNode<K, V>* rc = NULL);

    static void* operator new(size_t size) {
        return node_allocator<node_type>::talloc();
    }

    AVLNode<K, V>* copy();
    inline void update();
    inline void collect();
};

template<typename K, typename V>
inline void AVLNode<K, V>::collect() {
    this->entry.~entry_type();
    node_allocator<node_type>::collect(this);
}

template<typename K, typename V, typename AugmOp>
inline void AugmentedAVLNode<K, V, AugmOp>::collect() {
    this->entry.~entry_type();
    node_allocator<node_type>::collect(this);
}

template<typename K, typename V>
AVLNode<K, V>* AVLNode<K, V>::copy() {
    AVLNode* ret = new node_type(this->entry, lc, rc);
    increase(lc);
    increase(rc);
    return ret;
}

template<typename K, typename V, typename AugmOp>
AVLNode<K, V>* AugmentedAVLNode<K, V, AugmOp>::copy() {
    AVLNode<K, V>* ret = new node_type(this->entry, this->lc, this->rc);
    increase(this->lc);
    increase(this->rc);
    return ret; 
}

template<typename K, typename V>
inline size_t getHeightOrSize(AVLNode<K, V>* t) {
    return t ? t->heightOrSize : 0;
}

template<typename K, typename V>
inline size_t compareValue(AVLNode<K, V>* t) {
    return getHeightOrSize(t);
}

template<typename K, typename V>
inline bool isTooHeavy(AVLNode<K, V>* t1, AVLNode<K, V>* t2) {
    return getHeightOrSize(t1) > getHeightOrSize(t2) + 1;
}

template<typename K, typename V>
inline bool isBalanced(AVLNode<K, V>* t) {
    return !t || abs(getHeightOrSize(t->lc) - getHeightOrSize(t->rc)) < 2;
}

template<typename K, typename V>
inline bool is_single_rotation(AVLNode<K, V>* t, bool dir) {
    bool heavier = getHeightOrSize(t->lc) > getHeightOrSize(t->rc);
    return dir ? heavier : !heavier;
}

template<typename K, typename V>
inline void AVLNode<K, V>::update() {
    heightOrSize = max(getHeightOrSize(lc), getHeightOrSize(rc)) + 1;
    this->node_cnt = 1 + getNodeCount(lc) + getNodeCount(rc); 
}

template<typename K, typename V, typename AugmOp>
inline void AugmentedAVLNode<K, V, AugmOp>::update() {
    AVLNode<K, V>::update();

    accumulation = this->get_value();
    if (this->lc) accumulation = reduce_operator(accumulation, ((node_type*)this->lc)->accumulation);
    if (this->rc) accumulation = reduce_operator(accumulation, ((node_type*)this->rc)->accumulation);
}

template<typename K, typename V>
AVLNode<K, V>::AVLNode(entry_type entry, AVLNode* left, AVLNode* right) {
    this->entry = entry;
    this->ref_cnt = 1, lc = left, rc = right;
    update();
}

template<typename K, typename V, typename AugmOp>
AugmentedAVLNode<K, V, AugmOp>::AugmentedAVLNode(entry_type entry, AVLNode<K, V>* lc, AVLNode<K, V>* rc)
    : AVLNode<K, V>(entry, lc, rc) {
    reduce_operator = AugmOp();
    update();
}

template<typename K, typename V>
AVLNode<K, V>* persist_rebalanceSingle(AVLNode<K, V>* t) {
    AVLNode<K, V>* ret = t;
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
AVLNode<K, V>* t_join(AVLNode<K, V>* t1, AVLNode<K, V>* t2, AVLNode<K, V>* k) {
    AVLNode<K, V>* tmp = NULL;

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
AVLNode<K, V>* t_insert(AVLNode<K, V>* t, const pair<K, V>& elem){
    if (!t) return new AVLNode<K, V>(elem);

    AVLNode<K, V>* tmp;

    if (t->get_key() < elem.first) {
        if (t->ref_cnt >= 2) {
            tmp = t->copy();
            tmp->rc = t_insert(t->rc, elem);
            decrease_recursive(t);
        } else  {
            tmp = t;
            tmp->rc = t_insert(t->rc, elem);
        }
    } else if (t->get_key() > elem.first) {
        if (t->ref_cnt >= 2) {
            tmp = t->copy();
            tmp->lc = t_insert(tmp->lc, elem);
            decrease_recursive(t);
        } else {
            tmp = t;
            tmp->lc = t_insert(tmp->lc, elem);
        }
    } else if (t->get_key() == elem.first) {
        if (t->ref_cnt >= 2) {
            tmp = t->copy();
            decrease_recursive(t);
        } else
            tmp = t;

        tmp->set_value(elem.second);
    }

    tmp->update();
    return persist_rebalanceSingle(tmp);
}

