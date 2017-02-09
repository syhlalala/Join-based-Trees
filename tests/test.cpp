#include "augmented_map.h"
#include <iostream>
#include <algorithm>

using namespace std;
typedef pair<int,int> par;

struct augSum {
  typedef int aug_t;
  static int from_entry(int k, int v) { return v;}
  static int combine(int a, int b) { return a + b;}
};

int main() {
  int n = 1000;
  tree_map<int, int>::init();
  tree_map<int, int> m1, m2;
  par *v = new par[n];

  for (int i = 0; i < n; i++) {
    v[i] = make_pair(2*i,1);
  }
  m1.build(v, v + n);

  for (int i = 0; i < n; ++i) {
    m2.insert(make_pair(2*i+1, 2*i+1));
  }

  cout << m1.size() <<  " " << m2.size() <<  endl;

  tree_map<int, int> m3 = map_union(m1, m2);

  cout << m3.size() << endl;

  tree_map<int, int>::finish();
	
  augmented_map<int, int, augSum>::init();

  augmented_map<int, int, augSum> m4;

  m4.build(v, v + n);

  cout << m4.size() << "," << m4.augVal() << "," << m4.aug_left(n) << endl;

}
