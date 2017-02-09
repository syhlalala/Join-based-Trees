#include <algorithm>
#include <iostream>
#include <vector>
#include <random>
#include <climits>
#include <cstdlib>
#include <iomanip>

#include <cilk/cilk_api.h>
#include "prange.h"
#include "gettime.h"

using namespace std;

using data_type = int;
using point_type = Point<data_type>;
using tuple_type = tuple<data_type, data_type, data_type, data_type>;

int str_to_int(char* str) {
    return strtol(str, NULL, 10);
}

std::mt19937_64& GetRandGen() {
    static thread_local std::random_device rd;
    static thread_local std::mt19937_64 generator(rd());
    return generator;
}


template<typename T>
T Rand(std::mt19937_64& rd, data_type a, data_type b);

template <>
double Rand<double>(std::mt19937_64& rd, data_type a, data_type b) {
    std::uniform_real_distribution<> dis(a, b);
    return dis(rd);
}

template<>
int Rand<int>(std::mt19937_64& rd, data_type a, data_type b) {
    std::uniform_int_distribution<> dis(a, b);
    return dis(rd);
}

int random_hash(int seed, int a, int rangeL, int rangeR) {
	if (rangeL == rangeR) return rangeL;
	a = (a+seed) + (seed<<7);
	a = (a+0x7ed55d16) + (a<<12);
	a = (a^0xc761c23c) ^ (a>>19);
	a = (a+0x165667b1) + (a<<5);
	a = (a+seed>>5) ^ (a<<9);
	a = (a+0xfd7046c5) + (a<<3);
	a = (a^0xb55a4f09) ^ (a>>16);
	a = a % (rangeR-rangeL) + rangeL;
	return a;
}

vector<point_type> generate_points(size_t n, data_type a, data_type b) {
    vector<point_type> ret(n);

    #pragma omp parallel for
    for (int i = 0; i < n; ++i) {
        ret[i].x = random_hash('x', i, a, b);
        ret[i].y = random_hash('y', i, a, b);
        ret[i].w = random_hash('w', i, a, b);
    }

   return ret;
}

vector<tuple_type> generate_queries(size_t q, data_type a, data_type b) {
    vector<tuple_type> ret(q);

    #pragma omp parallel for
    for (int i = 0; i < q; ++i) {
        data_type x1 = random_hash('q'*'x', i*2, a, b);
        data_type y1 = random_hash('q'*'y', i*2, a, b);
        data_type x2 = random_hash('q'*'x', i*2+1, a, b);
        data_type y2 = random_hash('q'*'y', i*2+1, a, b);
		if (x1 > x2) {
			data_type t = x1; x1 = x2; x2 = t;
		}
		if (y1 > y2) {
			data_type t = y1; y1 = y2; y2 = t;
		}
        ret[i] = tuple_type(x1, y1, x2, y2);
    }

    return ret;
}
void reset_timers() {
     reserve_tm.reset();
     deconst_tm.reset();
     prefix_tm.reset();
     sort_tm.reset();
     const_tm1.reset();
     const_tm2.reset();
}


void run(vector<point_type> points, size_t iteration, data_type min_val, data_type max_val) {
    timer t;
    t.start();
    RangeQuery<data_type> *r = new RangeQuery<data_type>(points);
    double tm = t.stop();
	
	int query_num = 1000000;
	vector<tuple_type> queries = generate_queries(query_num, min_val, max_val);
	
	timer t_query;
	t_query.start();
	for (int i = 0; i < query_num; i++) {
		r->query(std::get<0>(queries[i]), std::get<1>(queries[i]), std::get<2>(queries[i]), std::get<3>(queries[i]));
	}
	double tm_query = t_query.stop();
    
    deconst_tm.start();
    delete r;
    deconst_tm.stop();
	
	/*
    cout << "RESULT" << fixed << setprecision(3)
         << "\talgo=prange"
         << "\ttime=" << tm
         << "\treserve-time=" << reserve_tm.total()
         << "\tsort-time=" << sort_tm.total()
         << "\tblock-construction-time=" << const_tm1.total()
         << "\tprefix-sum-time=" << prefix_tm.total()
         << "\tsingle-consruction-time=" << const_tm2.total()
         << "\tdeconstruction-time=" << deconst_tm.total()
         << "\tn=" << points.size()
         << "\tp=" << __cilkrts_get_nworkers()
         << "\tmin-val=" << min_val
         << "\tmax_val=" << max_val
         << "\titeration=" << iteration
         << endl;*/
    cout << "RESULT" << fixed << setprecision(3)
         << "\t" << tm
         << "\t" << reserve_tm.total()
         << "\t" << sort_tm.total()
         << "\t" << const_tm1.total()
         << "\t" << prefix_tm.total()
         << "\t" << const_tm2.total()
         << "\t" << deconst_tm.total()
		 << "\t" << run_tm.total()
		 << "\t" << tm_query
         << "\t" << points.size()
         << "\t" << __cilkrts_get_nworkers()
         << "\t" << min_val
         << "\t" << max_val
         << "\t" << iteration
         << endl;
}


int main(int argc, char** argv) {

    if (argc != 5) {
        cout << "Invalid number of command line arguments" << std::endl;
        exit(1);
    }

    size_t n = str_to_int(argv[1]);
    data_type min_val  = str_to_int(argv[2]);
    data_type max_val  = str_to_int(argv[3]);

    size_t iterations = str_to_int(argv[4]);

    for (size_t i = 0; i < iterations; ++i) {
        vector<point_type> points = generate_points(n, min_val, max_val);
        run(points, i, min_val, max_val);
        reset_timers();
    }

    return 0;
}

