#pragma once

namespace pbbs {
  // the following parameter can be tuned
  constexpr const size_t _binary_search_base = 16;

  template <typename Index_Map, typename F> 
  size_t linear_search(Index_Map I, typename Index_Map::T v, const F& less) {
    for (size_t i = 0; i < I.size(); i++)
      if (!less(I[i],v)) return i;
    return I.size();
  }

  // return index to first key greater or equal to v
  template <typename Index_Map, typename F> 
  size_t binary_search(Index_Map I, typename Index_Map::T v, const F& less) {
    size_t start = 0;
    size_t end = I.size();
    while (end-start > _binary_search_base) {
      size_t mid = (end+start)/2;
      if (!less(I[mid],v)) end = mid;
      else start = mid + 1;
    }
    return start + linear_search(I.cut(start,end),v,less);
  }
}
