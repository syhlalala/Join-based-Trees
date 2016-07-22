#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <cilk/cilk_api.h>
#include <atomic>
#include <mutex>
#include <stack>
#include "stack.h"
#include "utils.h"

const int DEFAULT_NODE_COUNT = 500000;
const int LINE_LENGTH  = 32;
const int MAX_LIST_LENGTH = 1 << 14;
const int MAX_THREAD_COUNT = 100;

template<typename T>
union memblock {
    T data;
    memblock<T>* next;
};

template<typename T>
struct thread_list {
    size_t sz;
    memblock<T>* head;  
    memblock<T>* mid;
    int cache_line[LINE_LENGTH];
};

template <typename T>
class node_allocator {
private:
    static atomic_flag mem_lock;
    static stack< memblock<T>* > pool_roots;
    static lockfree_stack< memblock<T>* > global_stack;
    static thread_list<T>** local_list;

    static int thread_count;
    static int per_thread;
    static bool get_list(memblock<T>*&);
    static void add_list_from_pool(memblock<T>*);

public:
    static bool initialized;
    static T* talloc();
    static void collect(T*);
    static void reallocate(size_t n = DEFAULT_NODE_COUNT);
    
    static void init(size_t n = DEFAULT_NODE_COUNT);
    static void reserve(size_t n = DEFAULT_NODE_COUNT);
    static void finish();
};

template<typename T> atomic_flag node_allocator<T>::mem_lock = ATOMIC_FLAG_INIT; 
template<typename T> stack <memblock<T>*> node_allocator<T>::pool_roots;
template<typename T> lockfree_stack<memblock<T>*> node_allocator<T>::global_stack;
template<typename T> bool node_allocator<T>::initialized = false;
template<typename T> thread_list<T>** node_allocator<T>::local_list;
template<typename T> int node_allocator<T>::thread_count = MAX_THREAD_COUNT;
template<typename T> int node_allocator<T>::per_thread = MAX_LIST_LENGTH;


template<typename T>
inline bool is_full(T* t) {
    return t->sz >= MAX_LIST_LENGTH;
}

template<typename T>
inline bool node_allocator<T>::get_list(memblock<T>*& head) {
    bool rem = global_stack.pop(head);
    if (rem) return true;

    if (!mem_lock.test_and_set()) {
        if (global_stack.empty()) reallocate();
        mem_lock.clear();
    }

    return false;
}

template<typename T>
inline void node_allocator<T>::add_list_from_pool(memblock<T>* node ) {
    memblock<T> *start = node;
    memblock<T> *end   = node + per_thread - 1;

    while (node != end) {
        node->next = (node + 1);
        ++node;
    }
    node->next = NULL;
    global_stack.push(start);
}

template<typename T>
void node_allocator<T>::reallocate(size_t n) {
    size_t list_cnt = ceil(n / (double)per_thread);

    if (list_cnt < thread_count)
        list_cnt = thread_count;

    size_t lists = thread_count + list_cnt;
    lists += thread_count - (lists % thread_count);

    size_t alloc_nodes = lists * per_thread;

    memblock<T>* mempool;

    size_t need = alloc_nodes * sizeof(memblock<T>);
    if ( (mempool = (memblock<T>*)malloc(need)) == NULL) {
        fprintf(stderr, "Cannot allocate space");
        exit(1);
    }

    pool_roots.push(mempool);
    cilk_for (int i = 0; i < lists; ++i)
        add_list_from_pool(mempool + i * per_thread);
}

template<typename T>
void node_allocator<T>::init(size_t n) {
    if (initialized) return;

    thread_count = __cilkrts_get_nworkers();
    global_stack.init();

    reallocate(n);
    local_list = new thread_list<T>*[thread_count];

    for (int i = 0; i < thread_count; ++i) {
        local_list[i] = new thread_list<T>();
        while(!get_list(local_list[i]->head));
        local_list[i]->sz = per_thread;
    }

    initialized = true;
}

template<typename T>
void node_allocator<T>::reserve(size_t n) {
    if (!atomic_flag_test_and_set(&mem_lock)) {
	reallocate(n);
	atomic_flag_clear(&mem_lock);
    }
}

template<typename T>
void node_allocator<T>::finish() {
    if (!initialized) return;

    global_stack.clear();
    for (int i = 0; i < thread_count; ++i) {
        delete local_list[i];
    }
    delete[] local_list;
    
    while (!pool_roots.empty()) {
        free(pool_roots.top());
        pool_roots.pop();
    }
    
    initialized = false;
}

template<typename T>
void node_allocator<T>::collect(T* node) {
    memblock<T>* new_node = (memblock<T>*) node;
    int id = __cilkrts_get_worker_number();

    if (local_list[id]->sz == per_thread+1) {
      local_list[id]->mid = local_list[id]->head;
    } else if (local_list[id]->sz == 2*per_thread) {
        global_stack.push(local_list[id]->mid->next);
        local_list[id]->mid->next = NULL;
        local_list[id]->sz = per_thread;
    }

    new_node->next = local_list[id]->head;
    local_list[id]->head = new_node;
    local_list[id]->sz++;
}

template<typename T>
T* node_allocator<T>::talloc() {
    int id = __cilkrts_get_worker_number();

    if (!local_list[id]->sz)  {
        while (!get_list(local_list[id]->head));
        local_list[id]->sz = per_thread;
    }

    local_list[id]->sz--;
    memblock<T>* p = local_list[id]->head;
    local_list[id]->head = local_list[id]->head->next;

    return &p->data;
}

