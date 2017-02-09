#include <algorithm>
#include <iostream>
#include <cstring>
#include <vector>
#include <cctype>
#include <fstream>
#include <set>
#include "augmented_map.h"
#include "pbbs-include/gettime.h"
#include "pbbs-include/par_string.h"
#include "index.h"

using namespace std;

long str_to_long(char* str) {
    return strtol(str, NULL, 10);
}

using set_elt = inv_index::set_elt;
using map_elt = inv_index::map_elt;
//using str_ptr = inv_index::str_ptr;
using set_type = inv_index::set_type;
using map_type = inv_index::map_type;

pair<map_elt*,pbbs::words*> parse(string filename, size_t max_size) {
  startTime();
  pbbs::words *W = new pbbs::words(filename, pbbs::is_not_alpha, 0, max_size);
  nextTime("read words");
  size_t n = W->size();
  cout << "Words = " << n << endl;

  bool* start_flag = pbbs::new_array_no_init<bool>(n);
  start_flag[n-1] = 0;
  char doc[] = "doc";
  char ids[] = "id";
  
  parallel_for(size_t i = 0; i < n-1; i++) {
    if ((strcmp((*W)[i],doc) == 0) && strcmp((*W)[i+1],ids) == 0)
      start_flag[i] = 1;
    else start_flag[i] = 0;
  }
  pbbs::seq<size_t> I = pbbs::pack_index(start_flag,n);
  free(start_flag);
  cout << "Docs = " << I.n << endl;
  nextTime("find starts");
  
  map_elt* KV = pbbs::new_array_no_init<map_elt>(n);
  parallel_for (size_t i = 0; i < I.n; ++i) {
    size_t start = I.A[i];
    size_t end = (i == (I.n-1)) ? n : I.A[i+1];
    //size_t id = str_to_long((*W)[start+2]);
    for (size_t j = start; j < end; j++) {
      KV[j] = map_elt((*W)[j], set_elt(i,0));
    }
  }
  free(W->Strings);
  nextTime("create pairs");
  free(I.A);
  return make_pair(KV,W);
}

int main(int argc, char** argv) {

  size_t max_chars = 1000000;
  if (argc > 1) max_chars = str_to_long(argv[1]);

  //  string fname = "/usr0/home/data/wikipedia/wikipedia.txt";
  string fname = "/usr0/home/danielf/wikipedia.txt";
  pair<map_elt*,pbbs::words*> X = parse(fname, max_chars);
  map_elt* KV = X.first;
  size_t n = X.second->size();

  inv_index myIndex(KV, KV +n);
  nextTime("build");

  cout << "Unique words = " << myIndex.idx.size() << endl;
  cout << "Map: ";  map_type::print_allocation_stats();
  cout << "Set: ";  set_type::print_allocation_stats();

  delete[] KV;

  vector<pair<char*,set_type> >out;
  myIndex.idx.content(std::back_inserter(out));

  FILE* x = freopen("sol.out", "w", stdout);

  for (size_t i = 0; i < out.size(); i++) { 
    std::cout << (out[i].first) << " " << (out[i].second).size() << std::endl;
  }
  delete X.second;
  return 0;
}
