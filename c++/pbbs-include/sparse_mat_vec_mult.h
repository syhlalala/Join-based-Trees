using namespace std;

// multiply a compresses sparse row matrix
template <class s_size_t, class E, class Mult, class Add>
void mat_vec_mult(s_size_t* starts, s_size_t* columns, E* values,
		  E* in, E* out,
		  size_t n, Mult mult, Add add) {
  parallel_for (size_t i = 0; i < n; i++) {
    s_size_t s = starts[i];
    s_size_t e = starts[i+1];
    E sum = 0;
    s_size_t* cp = columns+s;
    s_size_t* ce = columns+e;
    E* vp = values+s; 
    while (cp < ce-1) {
      sum = add(sum,mult(in[*cp],*vp));
      cp++; vp++;
      sum = add(sum,mult(in[*cp],*vp));
      cp++; vp++;
    }
    if (cp < ce) 
      sum = add(sum,mult(in[*cp],(*vp)));
    out[i] = sum;
  }
}
