#include <cilk/cilk.h>
//#include <parallel/algorithm>
#include <algorithm>
#include <iostream>
#include <cstring>
#include <ctime>
#include <cstdio>
#include <set>
#include "tree_map.h"
#include "gettime.h"
#include <map>
using namespace std;

typedef pair<int, int> par;

enum Test { UNION = 1, INTERSECT, INSERT, BUILD, FILTER, DE_UNION, DE_INTERSECT, SPLIT, DIFFERENCE, SEQ_UNION };

struct mapped {
    int k, v;
    mapped(int _k, int _v) : k(_k), v(_v) {};
    mapped(){};

    bool operator < (const mapped& m) const {
        return k < m.k;
    }

    bool operator > (const mapped& m) const {
        return k > m.k;
    }

    bool operator == (const mapped& m) const {
        return k == m.k;
    }
};

int seed_1, seed_2;

vector<par> get_input(int n, int seed) {
    srand(seed);
    vector <par> v;

    int max_number = 10 * n;
    int curr = 0, cnt = 0;
    
    while (cnt != n) {
        int r = rand() % 20;
        curr += r;

        int x = curr;
        int y = rand() % max_number;
        v.push_back(make_pair(x, y));
        ++cnt;
    }

    return v;
}

void shuffle(vector<par>& v) {
    srand(unsigned(time(NULL)));

    int n = v.size();
    for (int i = 0; i < n; ++i) {
        int j = rand() % (n-i);
        swap(v[i], v[i+j]);
    }
}

tree_map<int,int> read_map(int n, int seed) {
    vector <par> v = get_input(n, seed);

    tree_map<int, int> ret;
    ret.build_fast(v.begin(), v.end());
    return ret;
}


void test_union(int n, int m, int rep) {
    TreapNode<int, int>::init(); // <-- needed for treaps
    
    while (rep--) {
        tree_map<int, int>::reserve(4*n);

        tree_map<int, int> m1 = read_map(n, seed_1);
        tree_map<int, int> m2 = read_map(m, seed_2);

        timer t;
        t.start();
        tree_map<int, int> m3 = map_union(m1, m2);
        double tm = t.stop();

        cout << "Union time: " << tm << endl;
    
        tree_map<int, int>::finish();
    }
}


void test_seq_union(int n, int m, int rep) {
    
    while (rep--) {
        tree_map<int, int>::reserve(4*n);
    
        vector<par> v1 = get_input(n, seed_1);
        vector<par> v2 = get_input(m, seed_2);

        tree_map<int, int> m1;
        m1.build_fast(v1.begin(), v1.end());
    
        tree_map<int, int> m2;
        m2.build_fast(v2.begin(), v2.end());

        timer t;
        t.start();
        tree_map<int, int> m3 = map_union(m1, m2);
        double tm = t.stop();

        cout << "Union time: " << tm << endl;
    
        set <int> s1, s2, sret;
        vector<mapped> vec1, vec2, ret;
    
        for (int i = 0; i < v1.size(); ++i) {
            vec1.push_back(mapped(v1[i].first, v1[i].second));
            s1.insert(v1[i].first);
        } 
    
        for (int i = 0; i < v2.size(); ++i) {
            vec2.push_back(mapped(v2[i].first, v2[i].second));
            s2.insert(v2[i].first);
        }
    
        timer t2;
        t2.start();
    
        set_union(vec1.begin(), vec1.end(), vec2.begin(), vec2.end(), back_inserter(ret));
    
        double tm2 = t2.stop();
        cout << "STL vector union: " << tm2 << endl;
    
        timer t3;
        t3.start();
    
        set_union(s1.begin(), s1.end(), s2.begin(), s2.end(), inserter(sret, sret.begin()));
    
        double tm3 = t3.stop();
        cout << "STL set union: " << tm3 << endl;
    
        tree_map<int, int>::finish();
    }
}

void test_destructive_union(int n, int m, int rep) {
    
    while (rep--) {
        tree_map<int, int>::reserve(4*n);

        tree_map<int, int> m1 = read_map(n, seed_1);
        tree_map<int, int> m2 = read_map(m, seed_2);

        timer t;
        t.start();
        tree_map<int, int> m3 = map_union(final(m1), final(m2));
        double tm = t.stop();

        cout << "Union time: " << tm << endl;
    
        tree_map<int, int>::finish();
    }
}


void test_intersect(int n, int m, int rep) {
    
    while (rep--) {
        tree_map<int, int>::reserve(4*n);

        tree_map<int, int> m1 = read_map(n, seed_1);
        tree_map<int, int> m2 = read_map(m, seed_2);

        timer t;
        t.start();
        tree_map<int, int> m3 = map_intersect(m1, m2);
        double tm = t.stop();

        cout << "Intersect time: " << tm << endl;
    
        tree_map<int, int>::finish();
    }
}


void test_destructive_intersect(int n, int m, int rep) {
   
    while (rep--) {
        tree_map<int, int>::reserve(4*n);

        tree_map<int, int> m1 = read_map(n, seed_1);
        tree_map<int, int> m2 = read_map(m, seed_2);

        timer t;
        t.start();
        tree_map<int, int> m3 = map_intersect(final(m1), final(m2));
        double tm = t.stop();

        cout << "Intersect time: " << tm << endl;
    
        tree_map<int, int>::finish();
    }
}

void test_insert(int n, int rep) {
    while (rep--) {
        tree_map<int, int>::reserve(2*n);

        vector<par> v = get_input(n, seed_1);
        shuffle(v);

        tree_map<int, int> m1, m2;

        timer t1;
        t1.start();
        for (int i = 0; i < v.size(); ++i) {
            m1.insert(v[i]);
        }

        double tm1 = t1.stop();
        cout << "Join time: " << tm1 << endl;

        timer t2;
        t2.start();
        for (int i = 0; i < v.size(); ++i) {
            m2.normal_insert(v[i]);
        }

        double tm2 = t2.stop();
        cout << "Usual time: " << tm2 << endl;
    
        map<int, int> m;
    
        timer t3;
        t3.start();
        for (int i = 0; i < v.size(); ++i) {
            m.insert(v[i]);
        }
    
        double tm3 = t3.stop();
        cout << "STL time: " << tm3 << endl;
    
        tree_map<int, int>::finish();
    }
}


void test_build(int n, int rep) {
    
    while (rep--) {
        tree_map<int, int>::reserve(3*n);

        vector<par> v = get_input(n, seed_1);

        tree_map<int, int> m, m2;

        shuffle(v);
    
        timer t, t2;
        t.start();
        m.build(v.begin(), v.end());
        double tm = t.stop();
        cout << "Unsorted Build time: " << tm << endl;
    
        t2.start();
        m2.build_fast(v.begin(), v.end());
        double tm2 = t2.stop();
        cout << "Presort Build with Join: " << tm2 << endl;
    
        tree_map<int, int>::finish();
    }
}


void test_filter(int n, int rep) {
    
    while (rep--) {
        tree_map<int, int>::reserve(2*n);

        tree_map<int, int> m = read_map(n, seed_1);

        timer t;
        t.start();
        tree_map<int, int> res = m.filter([] (par t) { return (t.second & 1) == 0; });
        double tm = t.stop();
        cout << "Filter time: " << tm << endl;

        tree_map<int, int>::finish();
    }
}

void test_split(int n, int rep) {
    
    while (rep--) {
        tree_map<int, int>::reserve(2*n);

        vector<par> v = get_input(n, seed_1);

        tree_map<int, int> m;
        m.build_fast(v.begin(), v.end());

        int key = v[v.size() / 2].first;

        timer t;
        t.start();

        pair<tree_map<int, int>, tree_map<int, int> > res = m.split(key);

        double tm = t.stop();
        cout << "Split time: " << tm << endl;

        tree_map<int, int>::finish();
    }
}

void test_difference(int n, int m, int rep) {
    
    while (rep--) {
        tree_map<int, int>::reserve(4*n);

        tree_map<int, int> m1 = read_map(n, seed_1);
        tree_map<int, int> m2 = read_map(m, seed_2);

        timer t;
        t.start();
        tree_map<int, int> m3 = map_difference(m1, m2);

        double tm = t.stop();
        cout << "Difference time: " << tm << endl;

        tree_map<int, int>::finish();
    }
}

int main ( int argc, char *argv[] ) {
    /*if (argc != 5) {
        fprintf(stderr, "Wrong number of arguments provided\n");
        exit(1);
    }*/

    int repeat;
	if (argc == 5) repeat = strtol(argv[4], NULL, 0); else repeat = 1;
    int tree_1 = strtol(argv[2], NULL, 0);
    int tree_2 = strtol(argv[3], NULL, 0);

    seed_1 = seed_2 = 42;
    if (tree_1 == tree_2) seed_2 = 137;

    int command = strtol(argv[1], NULL, 0);

    switch (command) {
        case UNION:
            test_union(tree_1, tree_2, repeat);
            break;
        case BUILD:
            test_build(tree_1, repeat);
            break;
        case INSERT:
            test_insert(tree_1, repeat);
            break;
        case INTERSECT:
            test_intersect(tree_1, tree_2, repeat);
            break;
        case FILTER:
            test_filter(tree_1, repeat);
            break;
        case DE_UNION:
            test_destructive_union(tree_1, tree_2, repeat);
            break;
        case DE_INTERSECT:
            test_destructive_intersect(tree_1, tree_2, repeat);
            break;
        case SPLIT:
            test_split(tree_1, repeat);
            break;
        case DIFFERENCE:
            test_difference(tree_1, tree_2, repeat);
            break;
        case SEQ_UNION:
            test_seq_union(tree_1, tree_2, repeat);
            break;
        default:
            fprintf(stderr, "No such test");
            break;
    }

    return 0;
}
