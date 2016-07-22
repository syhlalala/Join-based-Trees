#pragma once

#include "tree_map.h"
#include "set_operations.h"

template<typename K>
class tree_set;

template<typename K>
tree_set<K> set_union(tree_set<K>&, tree_set<K>&);

template<typename K>
tree_set<K> set_intersect(tree_set<K>&, tree_set<K>&);

template<typename K>
tree_set<K> set_difference(tree_set<K>&, tree_set<K>&);

template<typename K>
tree_set<K>& final(tree_set<K>&);


template <typename K>
class tree_set {
public:
    typedef K                           key_type;
    typedef maybe<K>                    maybe_key;
    typedef TreeNode<K, nill>           node_type;
    typedef tree_set<K>                 set_type;
    typedef pair<set_type, set_type>    set_pair;
    typedef node_allocator<node_type>   allocator;

    tree_set();
    tree_set(const tree_set<key_type>& m);
    ~tree_set();
    
    int size() const;
    void clear();
    bool empty();
    
    template<typename Iterator>
    void build(const Iterator begin, const Iterator end);
    
    template<typename OutIterator>
    void content(OutIterator);
    
    bool find(const key_type&);
    
    maybe_key next(const key_type&);
    maybe_key previous(const key_type&);
    
    int rank(const key_type&);
    key_type select(int);
    
    void insert(const key_type&);
    bool remove(const key_type&);
    
    set_type range(key_type, key_type);
    set_pair split(const key_type&);
    
    template<typename Func>
    set_type filter(const Func&);
    
    friend set_type& final<K>(set_type&);

    friend set_type set_union<K>(set_type&, set_type&);
    friend set_type set_difference<K>(set_type&, set_type&);
    friend set_type set_intersect<K>(tree_set<K>&, set_type&);
    
    static void init();
    static void reserve(size_t n);
    static void finish();
    
    set_type& operator = (const set_type& s);
    
    
private:
    template<typename Iterator>
    node_type* construct(const Iterator, const Iterator);

    template<typename OutIterator>
    void collect_entries(const node_type*, OutIterator&);

    tree_map<K, nill> _set;
};

template<typename K>
tree_set<K>::tree_set() {
    _set.root = NULL;
}

template<typename K>
tree_set<K>::tree_set(const set_type& s) {
    _set = s._set;
}

template<typename K>
tree_set<K>::~tree_set() {
    _set.clear();
}

template<typename K>
void tree_set<K>::reserve(size_t n) {
    if (allocator::initialized) {
        allocator::reserve(n);
    } else  
        allocator::init(n);
}

template<typename K>
void tree_set<K>::init() {
    if (!allocator::initialized) {
        allocator::init();
    }
}

template<typename K>
void tree_set<K>::finish() {
    if (allocator::initialized) {
        allocator::finish();
    }
}

template<typename K>
int tree_set<K>::size() const {
    return _set.size();
}

template<typename K>
bool tree_set<K>::empty() {
    return _set.empty();
}

template<typename K>
tree_set<K>& tree_set<K>::operator = (const set_type& s) {
    _set = s._set;
    return *this;
}

template<typename K>
bool tree_set<K>::find(const key_type& key) {
    node_type* curr = _set.root;
    
    bool found = false;
    while (curr) {
        if (curr->key < key) 
            curr = curr->rc;
        else if (curr->key < key) 
            curr = curr->lc;
        else {
            found = true;
            break;
        }
    }
    
    return found;
}

template<typename K>
template<typename OutIterator>
void tree_set<K>::collect_entries(const node_type* curr, OutIterator& out) {
    if (!curr) return;
    
    collect_entries(curr->lc, out);
    *out = curr->get_key(); ++out;
    collect_entries(curr->rc, out);
}

template<typename K>
template<typename OutIterator>
void tree_set<K>::content(OutIterator out) {
    collect_entries(_set.root, out);
}

template<typename K>
maybe<K> tree_set<K>::next(const key_type& key) {
    return _set.next(key);
}

template<typename K>
maybe<K> tree_set<K>::previous(const key_type& key) {
    return _set.previous(key);
}

template<typename K>
int tree_set<K>::rank(const key_type& key) {
    return _set.rank(key);
}

template<typename K>
K tree_set<K>::select(int rank) {
    return _set.select(rank);
}

template<typename K>
void tree_set<K>::insert(const key_type& key) {
    _set.insert(make_pair(key, nill()));
}

template<typename K>
bool tree_set<K>::remove(const key_type& key) {
    _set.remove(key);
}

template<typename K>
void tree_set<K>::clear() {
    _set.clear();
}

template<typename K>
template<typename Iterator>
typename tree_set<K>::node_type* tree_set<K>::construct(const Iterator lo, const Iterator hi) {
    if (lo > hi)
        return NULL;
    if (lo == hi) 
        return new node_type(make_pair(*lo, nill()));
    
    const Iterator mid = lo + (std::distance(lo, hi) >> 1);
    
    node_type* l = cilk_spawn this->construct(lo, mid);
    node_type* r = construct(mid+1, hi);
    cilk_sync;
    
    return t_union(l, r, get_left<nill>());
}

template<typename K>
tree_set<K>& final(tree_set<K>& s) {
    final(s._set);
}

template<typename K>
template<typename Iterator>
void tree_set<K>::build(const Iterator begin, const Iterator end) {
    clear();
    
    if (begin != end)
        _set.root = construct(begin, end-1);
}

template<typename K>
tree_set<K> tree_set<K>::range(key_type low, key_type high) {
    tree_set<K> ret;
    ret._set = _set.range(low, high);
    return ret;
}

template<typename K>
pair<tree_set<K>, tree_set<K> > tree_set<K>::split(const key_type& key) {
    pair<tree_map<K, nill>, tree_map<K, nill> > maps = _set.split(key);
    
    tree_set<K> s1, s2;
    s1._set = maps.first;
    s2._set = maps.second;
    
    return make_pair(s1, s2); 
}

template<typename K>
template<typename Func>
tree_set<K> tree_set<K>::filter(const Func& f) {
    tree_set<K> ret;
    ret._set = _set.filter([f] (pair<K, nill> p) { return f(p.first); });
    return ret;
}

template<typename K>
tree_set<K> set_union(tree_set<K>& s1, tree_set<K>& s2) {
    tree_set<K> ret;
    ret._set = map_union(s1._set, s2._set);
    return ret;
}

template<typename K>
tree_set<K> set_intersect(tree_set<K>& s1, tree_set<K>& s2) {
    tree_set<K> ret;
    ret._set = map_intersect(s1._set, s2._set);
    return ret;
}

template<typename K>
tree_set<K> set_difference(tree_set<K>& s1, tree_set<K>& s2) {
    tree_set<K> ret;
    ret._set = map_difference(s1._set, s2._set);
    return ret;
}

