#pragma once

#include "set_operations.h"
#include "types.h"

template<typename K, typename V>
using TreeNode = AVLNode<K, V>;

template<typename K, typename V>
class tree_map;

template<typename K>
class tree_set;

template<typename K, typename V>
tree_map<K, V> map_union(tree_map<K, V>&, tree_map<K, V>&);

template<typename K, typename V>
tree_map<K, V> map_intersect(tree_map<K, V>&, tree_map<K, V>&);

template<typename K, typename V>
tree_map<K, V> map_difference(tree_map<K, V>&, tree_map<K, V>&);

template<typename K, typename V, typename BinaryOp>
tree_map<K, V> map_union(tree_map<K, V>&, tree_map<K, V>&, BinaryOp);

template<typename K, typename V, typename BinaryOp>
tree_map<K, V> map_intersect(tree_map<K, V>&, tree_map<K, V>&, BinaryOp);

template<typename K, typename V>
tree_map<K, V>& final(tree_map<K, V>&);


template <typename K, typename V>
class tree_map {
public:
    typedef K                           key_type;
    typedef V                           value_type;
    typedef pair<K, V>                  tuple;
    typedef maybe<K>                    maybe_key;
    typedef maybe<V>                    maybe_value;
    typedef TreeNode<K, V>              node_type;
    typedef tree_map<K, V>              map_type;
    typedef pair<map_type, map_type>    map_pair;
    typedef node_allocator<node_type>   allocator;

    tree_map();
    tree_map(const map_type& m);
    ~tree_map();
    
    void clear();
    bool empty();
    int size() const;
    
    template<typename Iterator>
    void build(const Iterator, const Iterator);
    
    template<typename Iterator>
    void build_fast(const Iterator, const Iterator);
    
    template<typename OutIterator>
    void content(OutIterator);
    
    maybe_value find(const key_type&);

    maybe_key next(const key_type&);
    maybe_key previous(const key_type&);

    int rank(const key_type&);
    key_type select(int);
    
    void normal_insert(const tuple&);
    void insert(const tuple&);
    bool remove(const key_type&);
    
    map_type range(key_type, key_type);
    map_pair split(const key_type&);
    
    template<typename Func>
    map_type filter(const Func&);
    
    template<typename ResultType, typename Func>
    tree_map<key_type, ResultType> forall(const Func&);

    friend class tree_set<K>;
    
    template<typename A, typename B>
    friend class tree_map;
    
    friend map_type& final<K, V>(map_type&);
    
    friend map_type map_union<K, V>(map_type&, map_type&);
    friend map_type map_difference<K, V>(map_type&, map_type&);
    friend map_type map_intersect<K, V>(map_type&, map_type&);
    
    template<typename A, typename B, typename BinaryOp>
    friend tree_map<A, B> map_union(tree_map<A, B>&, tree_map<A, B>&, BinaryOp);

    template<typename A, typename B, typename BinaryOp>
    friend tree_map<A, B> map_intersect(tree_map<A, B>&, tree_map<A, B>&, BinaryOp);
    
    static void init();
    static void reserve(size_t);
    static void finish();
    
    map_type& operator = (const map_type& k);


public:  

    template<typename OutIterator>
    void collect_entries(const node_type*, OutIterator&);
    
    template<typename Iterator>
    node_type* construct(const Iterator, const Iterator);

    template<typename Iterator>
    node_type* fast_construct(const Iterator, const Iterator);
    
    const V* search(const node_type*, const K&);

    bool destroy;
    node_type* root;
};

template<typename K, typename V>
tree_map<K, V>::tree_map() {
    destroy = false;
    root = NULL;
}

template<typename K, typename V>
tree_map<K, V>::~tree_map() {
    clear();
    root = NULL;
}

template<typename K, typename V>
tree_map<K, V>::tree_map(const map_type& m) {
    root = m.root;
    increase(root);
}

template<typename K, typename V>
void tree_map<K, V>::reserve(size_t n) { 
    if (allocator::initialized) {
        allocator::reserve(n);
    } else  
        allocator::init(n);
}

template<typename K, typename V>
void tree_map<K, V>::init() {
    if (!allocator::initialized) {
        allocator::init();
    }
}

template<typename K, typename V>
void tree_map<K, V>::finish() {
    if (allocator::initialized) {
        allocator::finish();
    }
}

template<typename K, typename V>
int tree_map<K, V>::size() const {
    return getNodeCount(root);
}

template<typename K, typename V>
bool tree_map<K, V>::empty() {
    return root == NULL;
}

template<typename K, typename V>
tree_map<K, V>& tree_map<K, V>::operator = (const map_type& m) {
    if (this == &m) return *this;

    clear();
    root = m.root;
    increase(root);
    return *this;
}

template<typename K, typename V>
maybe<V> tree_map<K, V>::find(const key_type& key) {
    const V* found = search(root, key);
    
    if (found != NULL) {
        return maybe_value(*found);
    }
    return maybe_value();
}

template<typename K, typename V>
template<typename OutIterator>
void tree_map<K, V>::collect_entries(const node_type* curr, OutIterator& out) {
    if (!curr) return;
    
    collect_entries(curr->lc, out);
    *out = curr->get_entry(); ++out;
    collect_entries(curr->rc, out);
}

template<typename K, typename V>
template<typename OutIterator>
void tree_map<K, V>::content(OutIterator out) {
    collect_entries(root, out);
}

template<typename K, typename V>
maybe<K> tree_map<K, V>::next(const key_type& key) {
    node_type* curr = root;
    
    key_type ret {};    
    bool has = false;

    while (curr) {
        if (curr->get_key() > key) {
            has = true;
            ret = curr->get_key();
            curr = curr->lc;
        } else 
            curr = curr->rc;
    }
    
    return maybe_key(ret, has);
}

template<typename K, typename V>
maybe<K> tree_map<K, V>::previous(const key_type& key) {
    node_type* curr = root;

    key_type ret {};    
    bool has = false;
    
    while (curr) {      
        if (curr->get_key() < key) {
            has = true;
            ret = curr->get_key();
            curr = curr->rc;
        } else 
            curr = curr->lc;
    }
    
    return maybe_key(ret, has);
}

template<typename K, typename V>
int tree_map<K, V>::rank(const key_type& key) {
    node_type* curr = root;
    
    int ret = 0;
    while (curr) {
        if (curr->get_key() < key) {
            ret += 1 + getNodeCount(curr->lc);
            curr = curr->rc;
        } else 
            curr = curr->lc;
    }
    
    return ret;
}

template<typename K, typename V>
K tree_map<K, V>::select(int rank) {    
    node_type* curr = root;
    key_type ret{};
    
    while (curr) {
        int left_size = getNodeCount(curr->lc);
        
        if (rank > left_size) {
            rank -= left_size + 1;
            curr = curr->rc;
        } else if (rank < left_size) {
            curr = curr->lc;
        } else {
            ret = curr->get_key();
            break;
        }
    }
    
    return ret;
}

template<typename K, typename V>
const V* tree_map<K, V>::search(const node_type* curr, const key_type& key) {
    if (!curr) return NULL;
    
    if (key < curr->get_key()) {
        return search(curr->lc, key);
    } else if (key > curr->get_key()) {
        return search(curr->rc, key);
    } else {
        return &curr->get_value();
    }
}

template<typename K, typename V>
void tree_map<K, V>::normal_insert(const tuple& p) {
    root = t_insert(root, p);
}

template<typename K, typename V>
void tree_map<K, V>::insert(const tuple& p) {
    split_info<node_type> split = t_split(root, p.first); 
    root = t_join(split.first, split.second, new node_type(p));        
}

template<typename K, typename V>
bool tree_map<K, V>::remove(const key_type& key) {
    split_info<node_type> split = t_split(root, key);
    root = t_join2(split.first, split.second);
    return split.removed;
}

template<typename K, typename V>
void tree_map<K, V>::clear() {
    if (allocator::initialized) {
        decrease_recursive(root);
    }       
    root = NULL;
    destroy = false;
}

template<typename K, typename V>
template<typename Iterator>
TreeNode<K, V>* tree_map<K, V>::construct(const Iterator lo, const Iterator hi) {
    if (lo > hi)
        return NULL;
    if (lo == hi) 
        return new node_type(*lo);
    
    const Iterator mid = lo + (std::distance(lo, hi) >> 1);
    
    node_type* l = cilk_spawn this->construct(lo, mid);
    node_type* r = construct(mid+1, hi);
    cilk_sync;
    
    return t_union(l, r, get_left<V>());
}


template<typename K, typename V>
template<typename Iterator>
TreeNode<K, V>* tree_map<K, V>::fast_construct(const Iterator lo, const Iterator hi) {
    if (lo > hi)
        return NULL;
    if (lo == hi) 
        return new node_type(*lo);
    
    const Iterator mid = lo + (std::distance(lo, hi) >> 1);
    
    node_type* m = new node_type(*mid);
    node_type* l = cilk_spawn this->fast_construct(lo, mid-1);
    node_type* r = fast_construct(mid+1, hi);
    cilk_sync;
    
    return t_join(l, r, m);
}

template<typename K, typename V>
template<typename Iterator>
void tree_map<K, V>::build_fast(const Iterator begin, const Iterator end) {
    
#ifdef GLIBCXX_PARALLEL
    clear();
    __gnu_parallel::sort(begin, end);
    if (begin != end)
      root = fast_construct(begin, end-1);
#else
    build(begin, end);
#endif
}

template<typename K, typename V>
template<typename Iterator>
void tree_map<K, V>::build(const Iterator begin, const Iterator end) {
    clear();
    
    if (begin != end)
        root = construct(begin, end-1);
}

template<typename K, typename V>
tree_map<K, V>& final(tree_map<K, V>& m) {
    m.destroy = true;
    return m;
}

template<typename K, typename V>
tree_map<K, V> tree_map<K, V>::range(key_type low, key_type high) {
    increase(root);
    
    maybe_key prev_key = previous(low);
    maybe_key next_key = next(high);
    
    if (prev_key) low = *prev_key;
    if (next_key) high = *next_key;
    
    split_info<node_type> left_split = t_split(root, low);
    decrease_recursive(left_split.first);
    
    split_info<node_type> right_split = t_split(left_split.second, high);
    decrease_recursive(right_split.second);
    
    tree_map<K, V> ret;
    ret.root = right_split.first;
    return ret;
}

template<typename K, typename V>
pair<tree_map<K, V>, tree_map<K, V> > tree_map<K, V>::split(const key_type& key) {
    increase(root);
    
    split_info<node_type> split = t_split(root, key);
    
    tree_map<K, V> ret_left, ret_right;
    ret_left.root = split.first;
    ret_right.root = split.second;
    
    return make_pair(ret_left, ret_right);
}

template<typename K, typename V>
template<typename Func>
tree_map<K, V> tree_map<K, V>::filter(const Func& f) {
    increase(root);
    
    map_type ret;
    ret.root = t_filter(root, f);
    return ret;
}

template<typename K, typename V>
template<typename ResultType, typename Func>
tree_map<K, ResultType> tree_map<K, V>::forall(const Func& f) {
    typedef TreeNode<K, ResultType> new_node_type;
    
    tree_map<K, ResultType> ret;
    t_forall(root, f, ret.root);
    return ret;
}

template<typename K, typename V>
tree_map<K, V> map_union(tree_map<K, V>& m1, tree_map<K, V>& m2) {
    return map_union(m1, m2, get_left<V>());
}

template<typename K, typename V>
tree_map<K, V> map_intersect(tree_map<K, V>& m1, tree_map<K, V>& m2) {
    return map_intersect(m1, m2, get_left<V>());
}

template<typename K, typename V>
tree_map<K, V> map_difference(tree_map<K, V>& m1, tree_map<K, V>& m2) {
    tree_map<K, V> ret;
    
    if (!m1.destroy) increase(m1.root); 
    if (!m2.destroy) increase(m2.root);
    
    ret.root = t_difference(m1.root, m2.root);
    
    if (m1.destroy) m1.root = NULL;
    if (m2.destroy) m2.root = NULL;
    
    return ret;
}

template<typename K, typename V, typename BinaryOp>
tree_map<K, V> map_union(tree_map<K, V>& m1, tree_map<K, V>& m2, BinaryOp op) {
    tree_map<K, V> ret;
    
    if (!m1.destroy) increase(m1.root); 
    if (!m2.destroy) increase(m2.root);
    
    ret.root = t_union(m1.root, m2.root, op);
    
    if (m1.destroy) m1.root = NULL;
    if (m2.destroy) m2.root = NULL;
    
    return ret;
}

template<typename K, typename V, typename BinaryOp>
tree_map<K, V> map_intersect(tree_map<K, V>& m1, tree_map<K, V>& m2,  BinaryOp op) {
    tree_map<K, V> ret;
    
    if (!m1.destroy) increase(m1.root); 
    if (!m2.destroy) increase(m2.root);
        
    ret.root = t_intersect(m1.root, m2.root, op);
    
    if (m1.destroy) m1.root = NULL;
    if (m2.destroy) m2.root = NULL;
    
    return ret;
}

