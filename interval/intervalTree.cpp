#include "augmented_map.h"
#include <iostream>
#include <algorithm>
#include "pbbs-include/random.h"

using namespace std;
typedef pair<int,int> par;

struct interval_map {
  using point = int;

  using interval = pair<point, point>;
  struct aug {
  using aug_t = interval;
    static aug_t get_empty() {
      return interval(0, 0);}
    static aug_t from_entry(point k, point v) {
      return interval(k, v);}
    static aug_t combine(aug_t a, aug_t b) {
      return (a.second > b.second) ? a : b;}
  };

  using amap = augmented_map<point,point,aug>;
  amap m;

  interval_map(int n) {
    amap::reserve(n); 
  }

  void finish() {
    amap::finish();
  }

  interval_map(interval* A, int n) {
    m = amap(A,A+n); }

  bool stab(point p) {
    return (m.aug_left(p).second > p);}

  void insert(interval i) {m.insert(i);}

  vector<interval> report_all(point p) {
    vector<interval> vec;
    amap a = m;
    interval I = a.aug_left(p);
    while (I.second > p) {
      vec.push_back(I);
      a.remove(I.first); 
      I = a.aug_left(p); }
    return vec; }

  void remove_small(int l) {
    auto f = [&] (interval I) {
      return (I.second - I.first >= l);};
    m.filter(f); }
};

long str_to_long(char* str) {
    return strtol(str, NULL, 10);
}

int main(int argc, char** argv) {
  size_t n = 100000000;
  size_t rounds = 5;
  if (argc > 1) n = str_to_long(argv[1]);
  if (argc > 2) rounds = str_to_long(argv[2]);
  par *v = new par[n];
  par *vv = new par[n];
  size_t max_size = (((size_t) 1) << 31)-1;

  pbbs::random r = pbbs::default_random;
  parallel_for (size_t i = 0; i < n; i++) {
    int start = r.ith_rand(2*i)%(max_size/2);
    int end = start + r.ith_rand(2*i+1)%(max_size-start);
    v[i] = make_pair(start, end);
  }

  for (size_t i=0; i < rounds; i++) {
    parallel_for (size_t i = 0; i < n; i++) {
      vv[i] = v[i];
    }
	const size_t threads = __cilkrts_get_nworkers();
    //startTime();
    interval_map xtree(n);
    //nextTime();
    timer t;
    t.start();
    interval_map itree(vv,n);
    double tm = t.stop();
    cout << n << "\t" << tm << "\t" << threads << endl;
	timer tq;
	int q_num = 1000000;
	bool* result = new bool[q_num];
	int* queries = new int[q_num];
	for (int i = 0; i < q_num; i++) {
		queries[i] = r.ith_rand(6*i)%max_size;
	}
	tq.start();
	for (int i = 0; i < q_num; i++) {
		result[i] = itree.stab(queries[i]);
	}
	double tm2 = tq.stop();
	cout << tm2 << endl;
	itree.finish();
  }

}
