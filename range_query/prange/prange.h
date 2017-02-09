#include <algorithm>
#include <iostream>
#include <cstring>
#include <vector>
#include <parallel/algorithm>
#include "tree_map.h"
#include "augmented_map.h"
#include "gettime.h"


using namespace std;

timer reserve_tm, deconst_tm;
timer sort_tm;
timer const_tm1, const_tm2;
timer prefix_tm, run_tm;

template<typename value_type>
struct Point {
    value_type x, y, w;
    Point(value_type _x, value_type _y, value_type _w) : x(_x), y(_y), w(_w){}

    bool operator < (const Point& p) const {
        return x < p.x;
    }

    Point(){}
};

template<typename value_type>
struct sum_op {
    value_type operator() (value_type a, value_type b) {
        return a + b;
    }
};

/**
 * Prallelism notes:
 *
 * Cilk workers set through CILK_NWORKERS variable
 * OMP threads need to be set through OMP_NUM_THREADS variable
 */
template<typename value_type>
struct RangeQuery {
    /** Should be larger than the maximum number of points */
    const value_type inf = std::numeric_limits<value_type>::max();

    using point_type = Point<value_type>;
    using par = pair<value_type, size_t>;
    using sum_map = augmented_map<int, value_type, sum_op<value_type> >;

    RangeQuery(vector<point_type>& points) {
        construct(points);
    }

    ~RangeQuery() {
        delete[] element_tree;
    }

    void construct(vector<point_type>& points) {
        const size_t n = points.size();

        reserve_tm.start();
        sum_map::reserve(50 * n);
        reserve_tm.stop();
        
        run_tm.start();
        // Sort points by the x-coordinate
        sort_tm.start();
        __gnu_parallel::sort(points.begin(), points.end());
        sort_tm.stop();

        xs.resize(n);
        vector<pair<int, size_t> > ys(n);

        for (size_t i = 0; i < n; ++i) {
            ys[i] = make_pair(points[i].y, points[i].w);
            xs[i] = points[i].x;
        }

        const size_t factor = 2;
        const size_t threads = __cilkrts_get_nworkers()*factor;
        const size_t block_size = max( (int)(n / threads), 1);

        sum_map* const trees = new sum_map[threads + 1];

        const_tm1.start();
        // Construct trees from contiguous blocks
        for (size_t i = 0; i < threads-1; ++i) {
            const size_t l = i * block_size;
            const size_t r = (i + 1 < threads) ? l + block_size : n;

            trees[i + 1].build(ys.begin() + l, ys.begin() + r);
        }
        const_tm1.stop();

        prefix_tm.start();
        // Compute the prefix sum over the blocks
        for (size_t i = 1; i <= threads-1; ++i) {
            trees[i] = map_union(final(trees[i]), trees[i-1]);
        }
        prefix_tm.stop();

        element_tree = new sum_map[n + 1];

        const_tm2.start();
        // Construct a prefix tree for each point in parallel
        cilk_for (size_t k = 0; k < threads; ++k) {
            const size_t l = k * block_size;
            const size_t r = (k + 1 < threads) ? l + block_size : n;

            element_tree[l] = trees[k];
            for (size_t j = l; j < r; ++j) {
                element_tree[j + 1] = element_tree[j];
                element_tree[j + 1].insert(make_pair(ys[j].first, ys[j].second));
            }
        }
        const_tm2.stop();
        run_tm.stop();

        delete[] trees;
    }

    value_type query(const value_type x1, const value_type y1, const value_type x2, const value_type y2) {

        const size_t lo = (size_t)(lower_bound(xs.begin(), xs.end(), x1) - xs.begin());
        const size_t hi = (size_t)(upper_bound(xs.begin(), xs.end(), x2) - xs.begin());

        if (lo >= hi) {
            return 0;
        } else {
            return element_tree[hi].reportRange(y1, y2) - element_tree[lo].reportRange(y1, y2);
        }
    }


    /** Stores the sorted x-cooridnates of the input points */
    vector<value_type> xs;

    /** The i-th tree represents the first i points, ordered by the x-coordinate */
    sum_map* element_tree;

};
