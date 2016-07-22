#pragma once

using namespace std;

template<typename K, typename V>
class AbstractNode {
public:
    typedef K key_type;
    typedef V value_type;
    typedef pair<K, V> entry_type;

    entry_type entry;

    size_t node_cnt;
    int ref_cnt;

    const entry_type& get_entry() const {
        return entry;
    }

    const K& get_key() const {
        return entry.first;
    }

    const V& get_value() const {
        return entry.second;
    }

    void set_value(const V value) {
        entry.second = value;
    }
};

