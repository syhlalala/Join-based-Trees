#pragma once

#include "augmented_map.h"
#include "tree_set.h"

class inv_index {
 public:
  struct str_less {
    bool operator() (char* s1, char* s2) const {
      return (strcmp(s1,s2) < 0);} };

  using word = char*;
  using doc_id = int;
  using set_elt = pair<doc_id, bool>;
  using map_elt = pair<word, set_elt>;
  using set_type = tree_map<doc_id, bool>;
  using map_type = tree_map<word, set_type, str_less>;

  map_type idx;

  inv_index(map_elt* start, map_elt* end) {
    size_t n = end - start;
    set_type::reserve((size_t) round(.45*n));
    map_type::reserve(n/300);
    nextTime("reserve");

    auto reduce = ([] (set_elt* start, set_elt* end) -> set_type
      {return set_type(start,end,false,true);});

    idx.build_reduce<set_elt>(start, end, reduce);
  }

  set_type* fetch_documents(std::string query[], size_t n) {
      set_type* docs = pbbs::new_array_no_init<set_type>(n);
      parallel_for (size_t i = 0; i < n; ++i) 
          pbbs::move_uninitialized(docs[i], *idx.find((word)query[i].c_str()));

      return docs;
  }

  set_type and_query(std::string query[], size_t n) {
      set_type* docs = fetch_documents(query, n);

      set_type res = pbbs::reduce(make_array_imap(docs, n), 
	 [] (set_type a, set_type b) {
            return map_intersect(std::move(a), std::move(b));});

      pbbs::delete_array(docs, n);
      return res;
  }

  set_type or_query(std::string query[], size_t n) {
      set_type* docs = fetch_documents(query, n);

      set_type res = pbbs::reduce(make_array_imap(docs,n), 
	 [] (set_type a, set_type b) {
            return map_union(std::move(a), std::move(b));});

      pbbs::delete_array(docs, n);
      return res;
  }
};
