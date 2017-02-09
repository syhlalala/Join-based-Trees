#pragma once
#include "utils.h"
#include "binary_search.h"

namespace pbbs {

  // the following parameter can be tuned
  constexpr const size_t _merge_base = 8196;

  template <class ImapA, class ImapB, class ImapR, class F> 
  void seq_merge(ImapA A, ImapB B, ImapR R, const F& f) {
    using T = typename ImapA::T;
    size_t nA = A.size();
    size_t nB = B.size();
    size_t i = 0;
    size_t j = 0;
    while (true) {
      if (i == nA) {
	while (j < nB) {R.update(i+j, B[j]); j++;}
	break; }
      if (j == nB) {
	while (i < nA) {R.update(i+j, A[i]); i++;}
	break; }
      T a = A[i];
      T b = B[j];
      if (f(a, b)) {
	R.update(i+j, a); 
	i++;
      } else {
	R.update(i+j, b); 
	j++;
      }
    }
  }
    
  template <class ImapA, class ImapB, class ImapR, class F> 
  void merge(ImapA A, ImapB B, ImapR R, const F& f) {
    size_t nA = A.size();
    size_t nB = B.size();
    size_t nR = nA + nB;
    if (nR < _merge_base) seq_merge(A, B, R, f);
    else if (nB > nA)  merge(B, A, R, f); 
    else {
      size_t mA = nA/2;
      size_t mB = binary_search(B, A[mA], f);
      size_t mR = mA + mB;
      par_do(true,
	     [&] () {merge(A.cut(0, mA), B.cut(0, mB), R.cut(0, mR), f);},
	     [&] () {merge(A.cut(mA, nA), B.cut(mB, nB), R.cut(mR, nR), f);});
    }
  }
}
