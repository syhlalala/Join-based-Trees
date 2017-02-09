#include "augmented_map.h"
#include <iostream>
#include <algorithm>
#include "../index/index.h"

using namespace std;

struct aug {
  typedef float aug_t;
  static float get_empty() { return 0.0;}
  static float from_entry(int k, int v) { return float(v)/2.0;}
  static float combine(float a, float b) { return a + b;}
};

using map  = augmented_map<int, int, aug>;
using elt = pair<int,int>;

void check(bool test, string message) {
  //cout << message << endl;
  if (!test) {
    cout << "Failed test: " << message << endl;
    exit(1);
  }
}

void test_map_more() {

  { // equality test
    elt a[3] = {elt(2,4), elt(4,5), elt(6,8)};
    elt b[2] = {elt(2,4), elt(4,5)};    
    map ma(a,a+3);
    map mb(b,b+2);
    check (!(ma == mb), "equality check, not equal");
    ma.remove(6);
    check (ma == mb, "equality check, equal");
  }
  
  { // self union test
    elt a[3] = {elt(2,4), elt(4,5), elt(6,8)};
    map ma(a,a+3);
    map mb = map_union(ma,ma);
    check (mb.size() == 3, "equality check, not equal");
  }  
  
  { // build combine test
    elt a[3] = {elt(2,4), elt(4,5), elt(2,6)};
    elt b[2] = {elt(2,10), elt(4,5)};
    map ma(a, a+3, [] (int a, int b) -> int {return a + b;});
    map mb(b,b+2);
    check (mb.size() == 2, "length check, combine test");
    check (ma.select(0).second == (4 + 6), "value check, combine test");
    check (ma == mb, "equality check, combine test");
  }  

  check(map::num_used_nodes() == 0, "used nodes at end of test_map_more");   
}

void test_map() {

  elt a[3] = {elt(2,4), elt(4,5), elt(6,8)};

  map ma(a,a+3);
  check(ma.size() == 3, "size check ma");

  check(*ma.find(4) == 5, "find check 4");
  check(!ma.find(3), "find check 3");

  check(*ma.next(3) == elt(4,5), "next check 3");
  check(*ma.next(4) == elt(6,8), "next check 4");
  check(!ma.next(6), "next check 6");
  check(*ma.previous(3) == elt(2,4), "previous check 3");
  check(*ma.previous(4) == elt(2,4), "previous check 4");
  check(!ma.previous(2), "previous check 2");

  check(ma.rank(4) == 1, "rank test 4");
  check(ma.rank(5) == 2, "rank test 5");
  check(ma.rank(7) == 3, "rank test 5");
  check(ma.select(0) == elt(2,4), "select test 0");
  check(ma.select(2) == elt(6,8), "select test 2");

  check(ma.aug_val() == 8.5, "aug test");
  check(ma.aug_left(4) == 4.5, "aug left test");
  check(ma.aug_right(4) == 6.5, "aug right test");
  check(ma.aug_range(3,11) == 6.5, "aug range test");

  ma.insert(elt(9,11));
  // ma == {elt(2,4), elt(4,5), elt(6,8), elt(9,11)};
  check(ma.size() == 4, "size check insert");
  check(*ma.find(9) == 11, "find check insert");
  
  ma.insert(elt(4,12));  // insert with overwrite
  // ma == {elt(2,4), elt(4,12), elt(6,8), elt(9,11)};
  check(ma.size() == 4, "size check insert second");
  check(*ma.find(4) == 12, "find check insert second");

  elt b[2] = {elt(1,6), elt(4,3)};

  map *mb_p = new map(b,b+2);
  check(mb_p->size() == 2, "size check mb");
  check(*(mb_p->find(4)) == 3, "find check mb");

  map mc = map_union(ma,*mb_p);
  // mc == {elt(1,6), elt(2,4), elt(4,12), elt(6,8), elt(9,11)};
  check(mc.size() == 5, "size check union");
  check(*ma.find(4) == 12, "find check union");
  
  auto mult = [] (int a, int b) { return a*b;};
  
  mc = map_union(ma,*mb_p,mult);
  // mc == {elt(1,6), elt(2,4), elt(4,36), elt(6,8), elt(9,11)};
  check(mc.size() == 5, "size check union with mult");
  check(*mc.find(4) == 36, "find check union with mult");
    
  map md = mc.range(2,4);
  // md == {elt(2,4), elt(4,36)};
  check(md.size() == 2, "size check after range");
  check(md.aug_val() == 20.0, "aug test after range");

  md = mc.range(0,7);
  // md == {elt(1,6), elt(2,4), elt(4,36), elt(6,8)};
  check(md.size() == 4, "size check after range 2");
  check(md.aug_val() == 27.0, "aug test after range 2");
  md.clear();

  auto is_odd = [] (elt a) { return (bool) (a.first & 1);};
    
  mc.filter(is_odd);
  // mc == {elt(1,6), elt(9,11)};
  check(mc.size() == 2, "size check after filter");
  check(mc.aug_val() == 8.5, "aug test after filter");

  //auto plus_one = [] (elt a) -> int { return a.second + 1;};

  // Not implemented yet
  //md = mc.forall(plus_one);
  // mc == {elt(1,7), elt(2,5), elt(4,37), elt(6,9), elt(9,12)};
  // mc == {elt(1,6), elt(9,11)};
  //check(md.size() == 5, "size check after forall");
  //check(md.aug_val() == 35.0, "aug test after forall");

  mc = map_intersect(ma,*mb_p);
  // mc == {elt(4,12)};
  check(mc.size() == 1, "size check intersect");
  check(*mc.find(4) == 12, "find check intersect");

  mc = map_intersect(ma,*mb_p,mult);
  // mc == {elt(4,36)};
  check(*mc.find(4) == 36, "find check intersect with mult");

  mc = map_difference(ma,*mb_p);
  // mc == {elt(2,4), elt(6,8), elt(9,11)};
  check(mc.size() == 3, "size check difference");
  check(mc.find(2), "find check after diff 2");
  check(mc.find(6), "find check after diff 6");
  check(!mc.find(4), "find check after diff 4");
  
  mc.clear();
  check(mc.size() == 0, "size check after clear");

  check(map::num_used_nodes() == 4 + 2, "used nodes"); 

  mc = map_union(std::move(ma),std::move(*mb_p));
  // mc == {elt(1,6), elt(2,4), elt(4,12), elt(6,8), elt(9,11)};
  check(mc.size() == 5, "size check union move");
  check(ma.size() == 0, "size check ma union move");
  check(mb_p->size() == 0, "size check mb union move");
  check(map::num_used_nodes() == 5, "used nodes after move");

  mc.remove(2);
  // mc == {elt(1,6), elt(4,12), elt(6,8), elt(9,11)};
  check(mc.size() == 4, "size check remove 2");
  check(!mc.find(2), "find check remove 2");

  mc.remove(3);
  // mc == {elt(1,6), elt(4,12), elt(6,8), elt(9,11)};
  check(mc.size() == 4, "size check remove 3");
  check(mc.aug_val() == 18.5, "aug test after remove 3");
  check(map::num_used_nodes() == 4, "used nodes after remove 3");
  
  mc.clear();
  check(map::num_used_nodes() == 0, "used nodes after clear");

  md.clear();
  for (size_t i = 0; i < 100; ++i) {
    ma.insert(make_pair(i, i));
    if (i < 30) mc.insert(make_pair(i, i));
    else md.insert(make_pair(i, i));
  }

  check(ma.size() == 100, "size check for ma insert");
  check(ma == map_union(std::move(mc), std::move(md)), 
    "check map equality");
  check(ma.contains(50), "check ma contains 50");
  check(!ma.contains(100), "check ma does not contain 100");

  ma.remove(50);
  check(ma.size() == 99, "size check for ma after delete");
  check(!ma.contains(50), "check ma does not contain 50 after delete");

  delete mb_p;
}    

void test_index() {
  // Test index
  using map_elt = inv_index::map_elt;
  using set_elt = inv_index::set_elt;
  using set_type = inv_index::set_type;
  //using map_type = inv_index::map_type;

  const size_t word_count = 12;
  const std::pair<const char*, size_t> words[] = { 
        {"apple", 0}, {"pie", 0}, {"delicious", 0}, 
        {"is", 1}, {"pumpkin", 1}, {"pie", 1}, {"tasty", 1} , 
        {"not", 2}, {"everyone", 2}, {"likes", 2}, {"apple", 2}, {"pie", 2}
  };

  map_elt* KV = pbbs::new_array_no_init<map_elt>(word_count);

  for (size_t i = 0; i < word_count; ++i)
    KV[i] = map_elt((char*)words[i].first, set_elt(words[i].second,0));

  inv_index index(KV, KV + word_count);

  std::string q1[] = {"apple", "pie"};
  set_type res1 = std::move(index.and_query(q1, 2));

  check(res1.size() == 2, "size check for and query result");
  check(res1.contains(0), "check if query result has document id 0");
  check(res1.contains(2), "check if query result has document id 2");

  std::string q2[] = {"tasty", "apple"};
  set_type res2 = std::move(index.or_query(q2, 2));
  
  check(res2.size() == 3, "size check for or query result");

  std::string q3[] = {"pumpkin"};
  set_type res3 = std::move(index.and_query(q3, 1));
  
  check(res3.size() == 1, "size check for and query result");
  check(res3.contains(1), "check if query result has document 1");

  free(KV);
}

void test_map_reserve_finish() {
  size_t n = 1000;
  for (size_t i = 0; i < 10; i++) {
    map::reserve(20);
    elt *a = new elt[n];
    elt *b = new elt[n];
    for (size_t i=0; i <n; i++) {
      a[i] = elt(n*i + rand()%n,0);
      b[i] = elt(n*i + rand()%n,0);
    }
    map ma(a,a+n);
    map mb(b,b+n);
    check(ma.size() == n, "size check map reserve");
    check(mb.size() == n, "size check map reserve");
    map::finish();
    delete[] a;
    delete[] b;
  }
} 

int main() {
  test_map();
  test_map_more();
  test_index();
  check(map::num_used_nodes() == 0, "used nodes at end");
  test_map_reserve_finish();
  cout << "passed!!" << endl;
}
