#include <algorithm>
#include <iostream>
#include <cstring>
#include <vector>
#include <parallel/algorithm>
#include "tree_operations.h"
#include "augmented_map.h"
#include "pbbs-include/get_time.h"

using namespace std;

timer init_tm, build_tm, total_tm, aug_tm, sort_tm, reserve_tm, outmost_tm, globle_tm, g_tm;
double early;

template<typename c_type, typename w_type>
struct Point {
    c_type x, y;
	w_type w;
    Point(c_type _x, c_type _y, w_type _w) : x(_x), y(_y), w(_w){}
    Point(){}
};

template<typename value_type>
inline bool inRange(value_type x, value_type l, value_type r) {
	return ((x>=l) && (x<=r));
}

template<typename c_type, typename w_type>
struct RangeQuery {
	using x_type = c_type;
	using y_type = c_type;
	using point_type = Point<c_type, w_type>;
	using point_x = pair<x_type, y_type>;
	using point_y = pair<y_type, x_type>;
	
	struct aug_add {
		typedef w_type aug_t;
		inline static aug_t from_entry(point_y k, w_type v) { return v; }
		inline static aug_t combine(aug_t a, aug_t b) { return a+b; }
		static aug_t get_empty() { return 0;}
	};
	using sec_aug = augmented_map<point_y, w_type, aug_add>;
	
	struct aug_union {
		typedef sec_aug aug_t;
		inline static aug_t from_entry(point_x k, w_type v) {
			return aug_t(make_pair(make_pair(k.second, k.first), v));
		}
		inline static aug_t combine(aug_t a, aug_t b) {
			return map_union(a, b, [](w_type x, w_type y) {return x+y;});
		}
		static aug_t get_empty() { return aug_t();}
	};

	using main_aug = augmented_map<point_x, w_type, aug_union>;
	using aug_node = typename main_aug::node_type;
	using sec_node = typename sec_aug::node_type;
	using main_entry = pair<point_x, w_type>;
	typedef tree_operations<sec_node>   tree_ops;
	
    RangeQuery(vector<point_type>& points) {
		construct(points);
    }

    ~RangeQuery() {
         main_aug::finish();
	 sec_aug::finish();
    }
	
    void construct(vector<point_type>& points) {
        const size_t n = points.size();
		
		reserve_tm.start();
		main_aug::reserve(n);
		sec_aug::reserve(22*n);
		reserve_tm.stop();
		
		pair<point_x, w_type> *pointsEle = new pair<point_x, w_type>[n];
		
		total_tm.start();
		
		init_tm.start();
		smallest = points[1].x;
		largest = points[1].x;
        cilk_for (size_t i = 0; i < n; ++i) {
			pointsEle[i] = make_pair(make_pair(points[i].x, points[i].y), points[i].w);
			//if (points[i].x > largest) largest = points[i].x;
			//if (points[i].x < smallest) smallest = points[i].x;
        }
		init_tm.stop();
		
		build_tm.start();
        range_tree = main_aug(pointsEle, pointsEle + n);
		build_tm.stop();
		total_tm.stop();
	//cout << "Outer Map: ";  main_aug::print_allocation_stats();
	//cout << "Inner Map: ";  sec_aug::print_allocation_stats();

		delete[] pointsEle;
    }
	
	w_type rep_l_sum(aug_node* r, x_type x, y_type y1, y_type y2) {
		w_type ans = 0;
		while (r) {
			if (!(x < r->get_key().first)) {
				if (r->lc) ans += r->lc->aug_val.aug_range(make_pair(y1, smallest), make_pair(y2, largest));
				if (inRange(r->get_key().second, y1, y2)) ans += r->get_value();
				r = r->rc;
				continue;
			}
			r = r->lc;
		}
		return ans;
	}
	
	w_type rep_r_sum(aug_node* r, x_type x, y_type y1, y_type y2) {
		w_type ans = 0;
		while (r) {
			if (!(r->get_key().first < x)) {
				if (r->rc) ans += r->rc->aug_val.aug_range(make_pair(y1, smallest), make_pair(y2, largest));
				if (inRange(r->get_key().second, y1, y2)) ans += r->get_value();
				r = r->lc;
				continue;
			}
			r = r->rc;
		}
		return ans;
	}

    w_type query_sum(const x_type x1, const y_type y1, const x_type x2, const y_type y2) {
		aug_node* r = range_tree.get_root();
		if (!r) return 0;
		w_type ans = 0;
		while (r) {
			if (x1 > r->get_key().first) {
				r = r->rc; continue;
			}
			if (x2 < r->get_key().first) {
				r = r->lc; continue;
			}
			if (inRange(r->get_key().second, y1, y2)) ans = r->get_value();
			ans += rep_r_sum(r->lc, x1, y1, y2) + rep_l_sum(r->rc, x2, y1, y2);
			break;
		}
		return ans;
    }
	
	template<typename OutIter>
	void rep_sec_l(sec_node* r, y_type y, OutIter& out) {
		while (r) {
			if (!(r->get_key().first > y)) {
				if (r->lc) {
					auto get = [] (sec_node* t) { return t->get_key(); };
					tree_ops::t_collect_seq(r->lc, out, get);
				}
				*out = r->get_key(); ++out; 
				r = r->rc; continue;
			}
			r = r->lc;
		}		
	}
	
	template<typename OutIter>
	void rep_sec_r(sec_node* r, y_type y, OutIter& out) {
		while (r) {
			if (!(r->get_key().first < y)) {
				if (r->rc) {
					auto get = [] (sec_node* t) { return t->get_key(); };
					tree_ops::t_collect_seq(r->rc, out, get);
				}
				*out = r->get_key(); ++out; 
				r = r->lc; continue;
			}
			r = r->rc;
		}		
	}
	
	template<typename OutIter>
	void rep_sec_range(sec_node* r, y_type y1, y_type y2, OutIter& out) {
		if (!r) return;
		while (r) {
			if (y1 > r->get_key().first) {
				r = r->rc; continue; }
			if (y2 < r->get_key().first) {
				r = r->lc; continue; }
			*out = r->get_key(); ++out;
			rep_sec_r(r->lc, y1, out);
			rep_sec_l(r->rc, y2, out);
			break;
		}
	}

	
	template<typename OutIter>
	void rep_r_points(aug_node* r, x_type x, y_type y1, y_type y2, OutIter& out) {
		while (r) {
			if (!(r->get_key().first < x)) {
				if (r->rc) {
					sec_node* sec_root = r->rc->aug_val.get_root();
					rep_sec_range(sec_root, y1, y2, out);
				}
				if (inRange(r->get_key().second, y1, y2)) {
					*out = r->get_key(); ++out; }
				r = r->lc; continue;
			}
			r = r->rc;
		}
	}
	
	template<typename OutIter>
	void rep_l_points(aug_node* r, x_type x, y_type y1, y_type y2, OutIter& out) {
		while (r) {
			if (!(r->get_key().first > x)) {
				if (r->lc) {
					sec_node* sec_root = r->lc->aug_val.get_root();
					rep_sec_range(sec_root, y1, y2, out);
				}
				if (inRange(r->get_key().second, y1, y2)) {
					*out = r->get_key(); ++out; }
				r = r->rc; continue;
			}
			r = r->lc;
		}
	}	
	
	template<typename OutIter>
	void query_points(const x_type x1, const y_type y1, const x_type x2, const y_type y2, OutIter out) {
		aug_node* r = range_tree.get_root();
		if (!r) return;
		while (r) {
			if (x1 > r->get_key().first) {
				r = r->rc; continue; }
			if (x2 < r->get_key().first) {
				r = r->lc; continue; }
			if (inRange(r->get_key().second, y1, y2)) {
				*out = make_pair(r->get_key().second, r->get_key().first); ++out;
			}
			rep_r_points(r->lc, x1, y1, y2, out);
			rep_l_points(r->rc, x2, y1, y2, out);
			break;
		}
	}
	
	c_type smallest, largest;
	main_aug range_tree;
};
