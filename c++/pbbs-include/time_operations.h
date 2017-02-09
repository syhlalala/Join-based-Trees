#include "get_time.h"
#include "sequence.h"
#include "utils.h"
#include "random.h"
#include "sample_sort.h"
#include "random_shuffle.h"
#include "merge.h"
#include "histogram.h"
#include "sparse_mat_vec_mult.h"
#include <iostream>
#include <ctype.h>
#include <math.h>

static timer bt;
using namespace std;

#define time(_var,_body)    \
  bt.start();               \
  _body;		    \
  double _var = bt.stop();

template<typename T, typename F>
T* new_array_f(size_t n, F f) {
  T* r = pbbs::new_array_no_init<T>(n);
  parallel_for (size_t i = 0; i < n; i++) r[i] = f(i);
  return r;
}

template<typename T>
T* new_array(size_t n, T v) {
  return new_array_f<T>(n, [v] (size_t i) {return v;});
}

template<typename T>
double t_tabulate(size_t n) {
  T* out = new_array<T>(n,0);
  time(t, parallel_for(size_t i = 0; i < n; i++) out[i] = i;);
  free(out);
  return t;
}

template<typename T>
double t_map(size_t n) {
  T* in = new_array<T>(n,1);
  T* out = new_array<T>(n,0);
  time(t, parallel_for(size_t i = 0; i < n; i++) out[i] = in[i];);
  free(in); free(out);
  return t;
}

template<typename T>
double t_reduce_add(size_t n) {
  T* in = new_array<T>(n,1);
  time(t, pbbs::reduce_add(make_array_imap(in, n)));
  free(in); 
  return t;
}

template<typename T>
double t_scan_add(size_t n) {
  T* in = new_array<T>(n,1);
  T* out = new_array<T>(n,0);
  time(t, pbbs::scan_add(make_array_imap(in, n), 
			 make_array_imap(out, n)););
  free(in); free(out);
  return t;
}

template<typename T>
double t_pack(size_t n) {
  bool* flags = new_array_f<bool>(n, [] (size_t i) -> bool {return i%2;});
  T* in = new_array_f<T>(n, [] (size_t i) {return i;});
  time(t, pbbs::pack(make_array_imap(in,n), 
		     make_array_imap(flags,n)););
  free(flags); free(in); 
  return t;
}

template<typename T>
double t_gather(size_t n) {
  pbbs::random r(0);
  T* in = new_array_f<T>(n, [&] (size_t i) {return i;});
  T* out = new_array<T>(n, 0);
  T* idx = new_array_f<T>(n, [&] (size_t i) {return r.ith_rand(i)%n;});
  time(t, parallel_for(size_t i=0; i<n; i++) {
      out[i] = in[idx[i]];});
  free(in); free(out); free(idx);
  return t;
}

template<typename T>
double t_scatter(size_t n) {
  pbbs::random r(0);
  T* out = new_array<T>(n, 0);
  T* idx = new_array_f<T>(n, [&] (size_t i) {return r.ith_rand(i)%n;});
  time(t, parallel_for(size_t i=0; i<n-3; i++) {
      out[idx[i]] = i;});
  free(out); free(idx);
  return t;
}

template<typename T>
double t_write_add(size_t n) {
  pbbs::random r(0);
  T* out = new_array<T>(n, 0);
  T* idx = new_array_f<T>(n, [&] (size_t i) {return r.ith_rand(i)%n;});
  time(t, parallel_for(size_t i=0; i<n-3; i++) {
      // prefetching useful on AMD, but not intel machines
      //__builtin_prefetch (&out[idx[i+3]], 1, 1);
      pbbs::write_add(&out[idx[i]],1);});
  free(out); free(idx);
  return t;
}

template<typename T>
double t_write_min(size_t n) {
  pbbs::random r(0);
  T* out = new_array<T>(n, n);
  T* idx = new_array_f<T>(n, [&] (size_t i) {return r.ith_rand(i)%n;});
  time(t, parallel_for(size_t i=0; i<n-3; i++) {
      //__builtin_prefetch (&out[idx[i+3]], 1, 1);
      pbbs::write_min(&out[idx[i]], (T) i, pbbs::less<T>());});
  free(out); free(idx);
  return t;
}

template<typename T>
double t_shuffle(size_t n) {
  T* in = new_array_f<T>(n, [&] (size_t i) {return i;});
  time(t, pbbs::random_shuffle(in,n););
  free(in); 
  return t;
}

template<typename T>
double t_histogram(size_t n) {
  pbbs::random r(0);
  T* in = new_array_f<T>(n, [&] (size_t i) {return r.ith_rand(i)%n;});
  T* out;
  time(t, out = pbbs::histogram<T>(in,n,n););
  free(in);
  free(out);
  return t;
}

template<typename T>
double t_histogram_few(size_t n) {
  pbbs::random r(0);
  T* in = new_array_f<T>(n, [&] (size_t i) {return r.ith_rand(i)%256;});
  T* out;
  time(t, out = pbbs::histogram<T>(in,n,256););
  free(in);
  free(out);
  return t;
}

template<typename T>
double t_histogram_same(size_t n) {
  T* in = new_array<T>(n, 10311);
  T* out;
  time(t, out = pbbs::histogram<T>(in,n,n););
  free(in);
  free(out);
  return t;
}


template<typename T>
double t_sort(size_t n) {
  pbbs::random r(0);
  T* in = new_array_f<T>(n, [&] (size_t i) {return r.ith_rand(i)%n;});
  time(t, pbbs::sample_sort(in,n,std::less<T>()););
  free(in); 
  return t;
}

template<typename T>
double t_count_sort_8(size_t n) {
  pbbs::random r(0);
  size_t num_buckets = (1<<8);
  size_t mask = num_buckets - 1;
  T* in = new_array_f<T>(n, [&] (size_t i) {return r.ith_rand(i);});
  auto rand_pos = [&] (size_t i) -> size_t {
    return in[i] & mask;};
  time(t, pbbs::count_sort(in,rand_pos,n,num_buckets););
  free(in); 
  return t;
}

template<typename T>
double t_merge(size_t n) {
  T* in1 = new_array_f<T>(n/2, [&] (size_t i) {return 2*i;});
  T* in2 = new_array_f<T>(n-n/2, [&] (size_t i) {return 2*i+1;});
  T* out = new_array<T>(n,0);
  time(t, pbbs::merge(make_array_imap(in1, n/2),
		      make_array_imap(in2, n-n/2), 
		      make_array_imap(out, n), 
		      std::less<T>()););
  free(in1); free(in2); free(out); 
  return t;
}

template<typename s_size_t, typename T>
double t_mat_vec_mult(size_t n) {
  pbbs::random r(0);
  size_t degree = 5;
  size_t nv = n/degree;
  s_size_t* starts = new_array_f<s_size_t>(nv+1, [&] (size_t i) 
					   {return degree*i;});
  s_size_t* columns = new_array_f<s_size_t>(n, [&] (size_t i) 
					    {return r.ith_rand(i)%nv;});
  T* values = new_array<T>(n,1);
  T* in = new_array<T>(nv,1);
  T* out = new_array<T>(nv,0);
  auto add = [] (T a, T b) { return a + b;};
  auto mult = [] (T a, T b) { return a * b;};

  time(t, mat_vec_mult(starts, columns, values, in, out, nv, mult, add););
  free(starts); free(columns); 
  free(in); free(out);
  return t;
}
