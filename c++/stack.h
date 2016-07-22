#include <atomic>

template<typename T>
class lockfree_stack{
public:
    struct Node {
        T value;
        Node* next;
        Node() : next(NULL) {};
    };

    struct ThreadList {
        Node* node;
        int cache_line[32];
        ThreadList() : node(NULL) {}

        void clear() {
            while (node) {
                Node* nxt = node->next;
                delete node;
                node = nxt;
            }
        }
    };
  
    class TaggedPointer{
    public:
        TaggedPointer(): m_node(NULL), m_counter(0) {}
 
        Node* Getnode(){
            return m_node.load(std::memory_order_acquire);
        }
 
        uint64_t GetCounter(){
            return m_counter.load(std::memory_order_acquire);
        }

        void reset() {
            m_node = NULL;
            m_counter = 0;
        }
 
        bool CompareAndSwap(Node* oldnode, uint64_t oldCounter, Node* newnode, uint64_t newCounter){
            bool cas_result;
            __asm__ __volatile__
            (
                "lock;"
                "cmpxchg16b %0;"
                "setz       %3;"
 
                : "+m" (*this), "+a" (oldnode), "+d" (oldCounter), "=q" (cas_result)
                : "b" (newnode), "c" (newCounter)
                : "cc", "memory"
            );
            return cas_result;
        }

    private:
         std::atomic<Node*> m_node;
         std::atomic<uint64_t> m_counter;
    }

    __attribute__((aligned(16)));

    lockfree_stack() {
        init();
    }

    void init() {
        if (initialized) return;

        m_head.reset();
        thread_cnt = __cilkrts_get_nworkers();

        free_list = new ThreadList*[thread_cnt];
        for (int i = 0; i < thread_cnt; ++i) {
            free_list[i] = new ThreadList();
        }

        initialized = true; 
    }

    void clear() {
        if (!initialized) return;

        for (int i = 0; i < thread_cnt; ++i) {
            free_list[i]->clear();
            delete free_list[i];
        }
        delete[] free_list;

        Node* head = m_head.Getnode(), *nxt;

        while (head) {
            nxt = head->next;
            delete head;
            head = nxt;
        }

        initialized = false;
    }

    bool empty() {
        return m_head.Getnode() == NULL;
    }
 
    Node* get_node() {
        int id = __cilkrts_get_worker_number();
        
        Node* node = free_list[id]->node;
        if (node != NULL) {
            free_list[id]->node = free_list[id]->node->next;
            return node;
        }
        
        return new Node();
     }
 
    void push(T entry){
        Node* oldHead;
        uint64_t oldCounter;
        
        Node* new_node = get_node(); 
        new_node->value = entry;
 
        do {
            oldHead = m_head.Getnode();
            oldCounter = m_head.GetCounter();
            new_node->next = oldHead;
        } while (!m_head.CompareAndSwap(oldHead, oldCounter, new_node, oldCounter + 1));
     }
 
    bool pop(T& entry) {
        int id = __cilkrts_get_worker_number();
        Node* oldHead;
        uint64_t oldCounter;
         
        do {
            oldHead =  m_head.Getnode();
            oldCounter = m_head.GetCounter();
            if(!oldHead) return false;
             
        } while (!m_head.CompareAndSwap(oldHead, oldCounter, oldHead->next, oldCounter + 1)); 
        
        entry = oldHead->value;
        oldHead->next = free_list[id]->node;
        free_list[id]->node = oldHead;
        return true;
    }

private:
    int thread_cnt;
    bool initialized = false;
    ThreadList** free_list;
    TaggedPointer m_head;
};

