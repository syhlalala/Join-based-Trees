#pragma once

#include "tree_map.h"
#include "set_operations.h"

template<typename K, typename V, typename AugmOp>
using AugmentedNode = AugmentedAVLNode<K, V, AugmOp>;

template<typename K, typename V, typename AugmOp>
class augmented_map;

template<typename K, typename V, typename AugmOp>
augmented_map<K, V, AugmOp> map_union(augmented_map<K, V, AugmOp>&, augmented_map<K, V, AugmOp>&);

template<typename K, typename V, typename AugmOp>
augmented_map<K, V, AugmOp> map_intersect(augmented_map<K, V, AugmOp>&, augmented_map<K, V, AugmOp>&);

template<typename K, typename V, typename AugmOp>
augmented_map<K, V, AugmOp> map_difference(augmented_map<K, V, AugmOp>&, augmented_map<K, V, AugmOp>&);

template<typename K, typename V, typename AugmOp, typename BinaryOp>
augmented_map<K, V, AugmOp> map_union(augmented_map<K, V, AugmOp>&, augmented_map<K, V, AugmOp>&, BinaryOp);

template<typename K, typename V, typename AugmOp, typename BinaryOp>
augmented_map<K, V, AugmOp> map_intersect(augmented_map<K, V, AugmOp>&, augmented_map<K, V, AugmOp>&, BinaryOp);

template<typename K, typename V, typename AugmOp>
augmented_map<K, V, AugmOp>& final(augmented_map<K, V, AugmOp>&);


template <typename K, typename V, typename AugmOp>
class augmented_map : public tree_map<K, V> {
public:
    typedef K                               key_type;
    typedef V                               value_type;
    typedef maybe<K>                        maybe_key;
    typedef maybe<V>                        maybe_value;
    typedef AugmOp                          operator_type;
    typedef pair<K, V>                      tuple;
    typedef AugmentedNode<K, V, AugmOp>     node_type;
    typedef augmented_map<K, V, AugmOp>     map_type;
    typedef pair<map_type, map_type>        map_pair;
    typedef node_allocator<node_type>       allocator;

    operator_type reduce_operator;

    augmented_map() : reduce_operator(AugmOp()) {}
    augmented_map(const map_type& m);
    ~augmented_map();
    
    void clear();
    void insert(const tuple&);

    void normal_insert(const tuple&);
    
    template<typename Iterator>
    void build(const Iterator begin, const Iterator end);

    template<typename Iterator>
    void build_fast(const Iterator begin, const Iterator end);

    map_type range(key_type, key_type);
    map_pair split(const key_type&);

    template<typename Func>
    map_type filter(const Func&);
    
    V accumulate(const key_type&);
    
    friend map_type& final<K, V, AugmOp>(map_type&);
    
    friend map_type map_union<K, V, AugmOp>(map_type&, map_type&);
    friend map_type map_difference<K, V, AugmOp>(map_type&, map_type&);
    friend map_type map_intersect<K, V, AugmOp>(map_type&, map_type&);
    
    template<typename A, typename B, typename C, typename BinaryOp>
    friend augmented_map<A, B, C> 
    map_union(augmented_map<A, B, C>&, augmented_map<A, B, C>&, BinaryOp);

    template<typename A, typename B, typename C, typename BinaryOp>
    friend augmented_map<A, B, C> 
    map_intersect(augmented_map<A, B, C>&, augmented_map<A, B, C>&, BinaryOp);
    
    static void init();
    static void reserve(size_t n);
    static void finish();
    
    map_type& operator = (const map_type& k);
    

private:
    template<typename Iterator>
    typename tree_map<K, V>::node_type* fast_construct(const Iterator begin, const Iterator end);

    template<typename Iterator>
    typename tree_map<K, V>::node_type* construct(const Iterator begin, const Iterator end);
};

template<typename K, typename V, typename AugmOp>
augmented_map<K, V, AugmOp>::~augmented_map() {
    this->clear();
    this->root = NULL;
}

template<typename K, typename V, typename AugmOp>
augmented_map<K, V, AugmOp>::augmented_map(const map_type& m) : tree_map<K, V>(m) {
    reduce_operator = m.reduce_operator;
}

template<typename K, typename V, typename AugmOp>
void augmented_map<K, V, AugmOp>::reserve(size_t n) { 
    if (allocator::initialized) {
        allocator::reserve(n);
    } else  
        allocator::init(n);
}

template<typename K, typename V, typename AugmOp>
void augmented_map<K, V, AugmOp>::init() { 
    if (!allocator::initialized) {
        allocator::init();
    }
}

template<typename K, typename V, typename AugmOp>
void augmented_map<K, V, AugmOp>::finish() { 
    if (allocator::initialized) {
        allocator::finish();
    }
}

template<typename K, typename V, typename AugmOp>
augmented_map<K, V, AugmOp>& augmented_map<K, V, AugmOp>::operator = (const map_type& m) {
    tree_map<K, V>::operator =(m);
    return *this;
}

template<typename K, typename V, typename AugmOp>
void augmented_map<K, V, AugmOp>::insert(const tuple& p) {
    split_info< typename tree_map<K, V>::node_type > split = t_split(this->root, p.first);  
    this->root = t_join(split.first, split.second, new node_type(p));
}

template<typename K, typename V, typename AugmOp>
void augmented_map<K, V, AugmOp>::normal_insert(const tuple& p) {
    this->root = t_insert(this->root, new node_type(p));
}


template<typename K, typename V, typename AugmOp>
template<typename Iterator>
typename tree_map<K, V>::node_type* augmented_map<K, V, AugmOp>::construct(const Iterator lo, const Iterator hi) {
    if (lo > hi)
        return NULL;
    if (lo == hi) 
        return new node_type(*lo);
    
    const Iterator mid = lo + (std::distance(lo, hi) >> 1);
    
    typename tree_map<K, V>::node_type* l = cilk_spawn this->construct(lo, mid);
    typename tree_map<K, V>::node_type* r = construct(mid+1, hi);
    cilk_sync;
    
    return t_union(l, r, get_left<V>());
}


template<typename K, typename V, typename AugmOp>
template<typename Iterator>
typename tree_map<K, V>::node_type* augmented_map<K, V, AugmOp>::fast_construct(const Iterator lo, const Iterator hi) {
    if (lo > hi)
        return NULL;
    if (lo == hi) 
        return new node_type(*lo);
    
    const Iterator mid = lo + (std::distance(lo, hi) >> 1);
    
    typename tree_map<K, V>::node_type* m = new node_type(*mid);
    typename tree_map<K, V>::node_type* l = cilk_spawn this->fast_construct(lo, mid-1);
    typename tree_map<K, V>::node_type* r = fast_construct(mid+1, hi);
    cilk_sync;
    
    return t_join(l, r, m);
}

template<typename K, typename V, typename AugmOp>
template<typename Iterator>
void augmented_map<K, V, AugmOp>::build_fast(const Iterator begin, const Iterator end) {
#ifdef GLIBCXX_PARALLEL
    clear();
    __gnu_parallel::sort(begin, end);
    if (begin != end)
      this->root = fast_construct(begin, end-1);
#else
    build(begin, end);
#endif
}


template<typename K, typename V, typename AugmOp>
template<typename Iterator>
void augmented_map<K, V, AugmOp>::build(const Iterator begin, const Iterator end) {
    clear();
    
    if (begin != end) {
        this->root = construct(begin, end-1);
   }
}

template<typename K, typename V, typename AugmOp>
V augmented_map<K, V, AugmOp>::accumulate(const key_type& key) {
    typename tree_map<K, V>::node_type* curr = this->root;
    
    V ret{};
    bool touch = 0;
    
    while (curr) {
        if (curr->get_key() <= key) {
            if (!touch){ 
                ret = curr->get_value();
                touch = 1;
            } else
                ret = reduce_operator(ret, curr->get_value());
        
            if (curr->lc) ret = reduce_operator(ret, ((node_type*)curr->lc)->accumulation);
            curr = curr->rc;
        } else 
            curr = curr->lc;
    }
    
    return ret;
}

template<typename K, typename V, typename AugmOp>
augmented_map<K, V, AugmOp>& final(augmented_map<K, V, AugmOp>& m) {
    m.tree_map<K, V>::destroy = true;
    return m;
}

template<typename K, typename V, typename AugmOp>
void augmented_map<K, V, AugmOp>::clear() {
    if (allocator::initialized) {
        decrease_recursive(this->root);
    }       
    this->root = NULL;
    this->destroy = false;
}

template<typename K, typename V, typename AugmOp>
augmented_map<K, V, AugmOp> augmented_map<K, V, AugmOp>::range(key_type low, key_type high) {
    increase(this->root);
    
    maybe_key prev_key = previous(low);
    maybe_key next_key = next(high);
    
    if (prev_key) low = *prev_key;
    if (next_key) high = *next_key;
    
    split_info<typename tree_map<K, V>::node_type> left_split = t_split(this->root, low);
    decrease_recursive(left_split.first);
    
    split_info<typename tree_map<K, V>::node_type> right_split = t_split(left_split.second, high);
    decrease_recursive(right_split.second);
    
    augmented_map<K, V, AugmOp> ret;
    ret.tree_map<K, V>::root = right_split.first;
    return ret;
}

template<typename K, typename V, typename AugmOp>
pair<augmented_map<K, V, AugmOp>, augmented_map<K, V, AugmOp> > augmented_map<K, V, AugmOp>::split(const key_type& key) {
    increase(this->root);
    
    split_info<typename tree_map<K, V>::node_type> split = t_split(this->root, key);
    
    augmented_map<K, V, AugmOp> ret_left, ret_right;
    ret_left.tree_map<K, V>::root = split.first;
    ret_right.tree_map<K, V>::root = split.second;
    
    return make_pair(ret_left, ret_right);
}

template<typename K, typename V, typename AugmOp>
template<typename Func>
augmented_map<K, V, AugmOp> augmented_map<K, V, AugmOp>::filter(const Func& f) {
    increase(this->root);
    
    map_type ret;
    ret.tree_map<K, V>::root = t_filter(this->root, f);
    return ret;
}

template<typename K, typename V, typename AugmOp>
augmented_map<K, V, AugmOp> map_union(augmented_map<K, V, AugmOp>& m1, augmented_map<K, V, AugmOp>& m2) {
    return map_union(m1, m2, get_left<V>());
}

template<typename K, typename V, typename AugmOp>
augmented_map<K, V, AugmOp> map_intersect(augmented_map<K, V, AugmOp>& m1, augmented_map<K, V, AugmOp>& m2) {
    return map_intersect(m1, m2, get_left<V>());
}

template<typename K, typename V, typename AugmOp>
augmented_map<K, V, AugmOp> map_difference(augmented_map<K, V, AugmOp>& m1, augmented_map<K, V, AugmOp>& m2) {
    augmented_map<K, V, AugmOp> ret;
    
    if (!m1.tree_map<K, V>::destroy) increase(m1.tree_map<K, V>::root); 
    if (!m2.tree_map<K, V>::destroy) increase(m2.tree_map<K, V>::root);
    
    ret.tree_map<K, V>::root = t_difference(m1.tree_map<K, V>::root, m2.tree_map<K, V>::root);
    
    if (m1.tree_map<K, V>::destroy) m1.tree_map<K, V>::root = NULL;
    if (m2.tree_map<K, V>::destroy) m2.tree_map<K, V>::root = NULL;
    
    return ret;
}

template<typename K, typename V, typename AugmOp, typename BinaryOp>
augmented_map<K, V, AugmOp> map_union(augmented_map<K, V, AugmOp>& m1, augmented_map<K, V, AugmOp>& m2, BinaryOp op) {
    augmented_map<K, V, AugmOp> ret;
    
    if (!m1.tree_map<K, V>::destroy) increase(m1.tree_map<K, V>::root); 
    if (!m2.tree_map<K, V>::destroy) increase(m2.tree_map<K, V>::root);
    
    ret.tree_map<K, V>::root = t_union(m1.tree_map<K, V>::root, m2.tree_map<K, V>::root, op);
    
    if (m1.tree_map<K, V>::destroy) m1.tree_map<K, V>::root = NULL;
    if (m2.tree_map<K, V>::destroy) m2.tree_map<K, V>::root = NULL;
    
    return ret;
}

template<typename K, typename V, typename AugmOp, typename BinaryOp>
augmented_map<K, V, AugmOp> map_intersect(augmented_map<K, V, AugmOp>& m1, augmented_map<K, V, AugmOp>& m2,  BinaryOp op) {
    augmented_map<K, V, AugmOp> ret;
    
    if (!m1.tree_map<K, V>::destroy) increase(m1.tree_map<K, V>::root); 
    if (!m2.tree_map<K, V>::destroy) increase(m2.tree_map<K, V>::root);
        
    ret.tree_map<K, V>::root = t_intersect(m1.tree_map<K, V>::root, m2.tree_map<K, V>::root, op);
    
    if (m1.tree_map<K, V>::destroy) m1.tree_map<K, V>::root = NULL;
    if (m2.tree_map<K, V>::destroy) m2.tree_map<K, V>::root = NULL;
    
    return ret;
}


