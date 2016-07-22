#include "abstract_node.h"
#include "commons.h"
#include "list_allocator.h"
#include <climits>

struct state {
    uint32_t seed;
    int cache_line[LINE_LENGTH];
};

state st[MAX_THREAD_COUNT];

size_t get_rand() {
    size_t id = __cilkrts_get_worker_number();
    return rand_r(&st[id].seed);
}

template<typename K, typename V>
class TreapNode : public AbstractNode<K, V> {
public:
    typedef typename AbstractNode<K, V>::entry_type     entry_type;
    typedef TreapNode<K, V>                             node_type;

    size_t priority;

    node_type* lc;
    node_type* rc;

    TreapNode() {}
    TreapNode(entry_type, TreapNode* left=NULL, TreapNode* right=NULL);
    TreapNode(entry_type, TreapNode*, TreapNode*, size_t);

    static void init() {
        srand(unsigned(time(NULL)));
        for (int i = 0; i < MAX_THREAD_COUNT; ++i) {
            st[i].seed = 1 + (rand() % INT_MAX);
        }
    }

    static void* operator new(size_t size) {
        return node_allocator<node_type>::talloc();
    }
    
    virtual node_type* copy();
    virtual inline void update();
    virtual inline void collect();
};

template<typename K, typename V, typename AugmOp>
class AugmentedTreapNode : public TreapNode<K, V> {
public:
    typedef typename AbstractNode<K, V>::entry_type     entry_type;
    typedef AugmentedTreapNode<K, V, AugmOp>            node_type;

    AugmOp reduce_operator;
    V accumulation;

    AugmentedTreapNode(entry_type, TreapNode<K, V>* lc = NULL, TreapNode<K, V>* rc = NULL);
    AugmentedTreapNode(entry_type, TreapNode<K, V>*, TreapNode<K, V>*, size_t);
    
    static void* operator new(size_t size) {
        return node_allocator<node_type>::talloc();
    }
    
    TreapNode<K, V>* copy();
    inline void update();
    inline void collect();
};

template<typename K, typename V>
inline void TreapNode<K, V>::collect() {
    this->entry.~entry_type();
    node_allocator<node_type>::collect(this);
}

template<typename K, typename V, typename AugmOp>
inline void AugmentedTreapNode<K, V, AugmOp>::collect() {
    this->entry.~entry_type();
    node_allocator<node_type>::collect(this);
}

template<typename K, typename V>
inline size_t compareValue(TreapNode<K, V>* t) {
    return !t ? 0 : -t->priority;
}

template<typename K, typename V>
TreapNode<K, V>* TreapNode<K, V>::copy() {
    TreapNode* ret = new node_type(this->entry, lc, rc, priority);
    increase(lc);
    increase(rc);
    return ret;
}

template<typename K, typename V, typename AugmOp>
TreapNode<K, V>* AugmentedTreapNode<K, V, AugmOp>::copy() {
    TreapNode<K, V>* ret = new node_type(this->entry, this->lc, this->rc, this->priority);
    increase(this->lc);
    increase(this->rc);
    return ret; 
}

template<typename K, typename V>
inline bool higherPriority(TreapNode<K, V>* t1, TreapNode<K, V>* t2) {
    if (!t1) return false;
    return !t2 || t1->priority >= t2->priority;
}

template<typename K, typename V>
bool isBalanced(TreapNode<K, V>* t) {
    return !t || (higherPriority(t, t->lc) && higherPriority(t, t->rc));
}

template<typename K, typename V>
inline void TreapNode<K, V>::update() {
    this->node_cnt = 1 + getNodeCount(lc) + getNodeCount(rc);
}

template<typename K, typename V, typename AugmOp>
inline void AugmentedTreapNode<K, V, AugmOp>::update() {
    TreapNode<K, V>::update();
    
    accumulation = this->value;
    if (this->lc) accumulation = reduce_operator(accumulation, ((node_type*)this->lc)->accumulation);
    if (this->rc) accumulation = reduce_operator(accumulation, ((node_type*)this->rc)->accumulation);
}

template<typename K, typename V>
TreapNode<K, V>::TreapNode(entry_type entry, TreapNode* left, TreapNode* right) {
    this->entry = entry;
    this->ref_cnt = 1;
    priority = get_rand();
    lc = left, rc = right;
    update();
}

template<typename K, typename V, typename AugmOp>
AugmentedTreapNode<K, V, AugmOp>::AugmentedTreapNode(entry_type entry, TreapNode<K, V>* left, TreapNode<K, V>* right) 
    : TreapNode<K, V>(entry, left, right) {
    reduce_operator = AugmOp();
    update();
}

template<typename K, typename V>
TreapNode<K, V>::TreapNode(entry_type entry, TreapNode* left, TreapNode* right, size_t prior) {
    this->entry = entry;
    this->ref_cnt = 1, priority = prior;
    lc = left, rc = right;
    update();
}

template<typename K, typename V, typename AugmOp>
AugmentedTreapNode<K, V, AugmOp>::AugmentedTreapNode(entry_type entry, TreapNode<K, V>* left, TreapNode<K, V>* right, size_t prior) 
    : TreapNode<K, V> (entry, left, right, prior) {
    reduce_operator = AugmOp();
    update();
}

template<typename K, typename V>
TreapNode<K, V>* t_join(TreapNode<K, V>* t1, TreapNode<K, V>* t2, TreapNode<K, V>* k) {
    TreapNode<K, V>* tmp = NULL;
    
    if ( higherPriority(k, t1) && higherPriority(k, t2) ) {
        tmp = k;
        tmp->lc = t1;
        tmp->rc = t2;
    } else if ( higherPriority(t1, t2) ) {
        if (t1->ref_cnt >= 2) {
            tmp = t1->copy();
            tmp->rc = t_join(t1->rc, t2, k);
            decrease_recursive(t1);
        } else {
            tmp = t1;
            tmp->rc = t_join(t1->rc, t2, k);
        }
    } else {
        if (t2->ref_cnt >= 2) {
            tmp = t2->copy();
            tmp->lc = t_join(t1, t2->lc, k);
            decrease_recursive(t2);
        } else {
            tmp = t2;
            tmp->lc = t_join(t1, t2->lc, k);
        }
    }

    tmp->update();
    return tmp; 
}

template<typename K, typename V>
TreapNode<K, V>* t_insert(TreapNode<K, V>* t, const pair<K, V>& elem) {}
