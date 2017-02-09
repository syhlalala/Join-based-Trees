//#define GLIBCXX_PARALLEL

#include <algorithm>
#include <iostream>
#include <vector>
#include <random>
#include <climits>
#include <cstdlib>
#include <iomanip>

#include <cilk/cilk_api.h>
#include "range_tree.h"
#include "pbbs-include/get_time.h"

using namespace std;

using data_type = int;
using point_type = Point<data_type, data_type>;
using tuple_type = tuple<data_type, data_type, data_type, data_type>;

struct Query_data {
	data_type x1, x2, y1, y2;
} query_data;

int str_to_int(char* str) {
    return strtol(str, NULL, 10);
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

    //#pragma omp parallel for
    cilk_for (int i = 0; i < n; ++i) {
        ret[i].x = random_hash('x', i, a, b);
        ret[i].y = random_hash('y', i, a, b);
        ret[i].w = random_hash('w', i, a, b);
    }

   return ret;
}

vector<Query_data> generate_queries(size_t q, data_type a, data_type b) {
    vector<Query_data> ret(q);

    //#pragma omp parallel for
    cilk_for (int i = 0; i < q; ++i) {
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
        ret[i].x1 = x1; ret[i].x2 = x2;
		ret[i].y1 = y1; ret[i].y2 = y2;
    }

    return ret;
}

void run(vector<point_type> points, size_t iteration, data_type min_val, data_type max_val, int query_num) {
	const size_t threads = __cilkrts_get_nworkers();
    timer t;
	t.start();
	RangeQuery<data_type, data_type> *r = new RangeQuery<data_type, data_type>(points);
	double tm = t.stop();

	vector<Query_data> queries = generate_queries(query_num, min_val, max_val);
	
	timer t_query_total;
	t_query_total.start();
	cilk_for (int i = 0; i < query_num; i++) {
		//r->query_sum(queries[i].x1, queries[i].y1, queries[i].x2, queries[i].y2);
		vector <pair<int, int> > out;
		r->query_points(queries[i].x1, queries[i].y1, queries[i].x2, queries[i].y2, std::back_inserter(out));
	}
	t_query_total.stop();
	
	
	cout << points.size() << "\t" << query_num << "\t" << threads << "\t";
	cout << reserve_tm.get_total() << "\t" 
	     << init_tm.get_total() << "\t"
	     << build_tm.get_total() << "\t"
	     << total_tm.get_total() << "\t"
	     << t_query_total.get_total() << endl;

	reserve_tm.reset();
	init_tm.reset(); sort_tm.reset(); build_tm.reset(); total_tm.reset(); 
	//cout << "aug time: " << aug_tm.total() << endl;
	//cout << "reserve time: " << reserve_tm.total() << endl;
	//cout << "sort time: " << sort_tm.total() << endl;
	//cout << "initialize time: " << init_tm.total() << endl;
	//cout << "build time: " << build_tm.total() << endl;
	//cout << "Run time: " << total_tm.total() << endl;
	//cout << "Query time: " << m_query << endl;

    delete r;
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
	//int query_num  = str_to_int(argv[5]);
	int query_num = 100;
	//srand(2016);
	
    for (size_t i = 0; i < iterations; ++i) {
        vector<point_type> points = generate_points(n, min_val, max_val);
        run(points, i, min_val, max_val, query_num);
    }

    return 0;
}

