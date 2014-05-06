#ifndef __febird_automata_graph_walker_hpp__
#define __febird_automata_graph_walker_hpp__

#include <boost/current_function.hpp>
#include <boost/noncopyable.hpp>
#include <febird/bitmap.hpp>
#include "dfa_interface.hpp"

#define ASSERT_isNotFree(s)       assert(!this->is_free(s))
#define ASSERT_isNotFree2(au,s)   assert(!au->is_free(s))

template<class VertexID>
class BFS_GraphWalker {
protected:
	size_t _M_depth = 0;
	size_t idx = 0;
	valvec<VertexID> q1, q2;
	simplebitvec     color;
public:
	void resize(size_t VertexNum) {
		assert(q1.size() == idx);
		assert(q2.empty());
		q1.reserve(std::min<size_t>(VertexNum, 512));
		q2.reserve(std::min<size_t>(VertexNum, 512));
		q1.erase_all();
		idx = 0;
		_M_depth = 0;
		color.resize_no_init(VertexNum);
		color.fill(0);
	}
	void put_root(VertexID root) {
		assert(root < color.size());
		q1.push_back(root);
		color.set1(root);
	}
	VertexID next() {
		assert(idx <= q1.size());
		if (q1.size() == idx) {
			q1.swap(q2);
			idx = 0;
			_M_depth++;
			q2.erase_all();
		}
		assert(!q1.empty());
		return q1[idx++];
	}
	template<class Graph>
	void putChildren(const Graph* g, VertexID parent) {
		assert(parent < GraphTotalVertexes(g));
		g->for_each_dest(parent,
			[this,g](VertexID, VertexID child) {
				assert(child < GraphTotalVertexes(g));
				ASSERT_isNotFree2(g, child);
				if (this->color.is0(child)) {
					this->q2.push_back(child);
					this->color.set1(child);
				}
			});
	}
	template<class Graph, class OnEdge>
	void putChildren2(const Graph* g, VertexID parent, OnEdge on_edge) {
		assert(parent < GraphTotalVertexes(g));
		g->for_each_dest(parent,
			[&](VertexID parent, VertexID child) {
				assert(child < GraphTotalVertexes(g));
				ASSERT_isNotFree2(g, child);
				if (this->color.is0(child)) {
					if (on_edge(parent, child, true)) {
						this->q2.push_back(child);
						this->color.set1(child);
					}
				} else
					on_edge(parent, child, false);
			});
	}
	bool is_finished() const { return q1.size() + q2.size() == idx; }
	size_t depth() const { return _M_depth; }
};

template<class VertexID, class ColorID = VertexID>
class BFS_MultiPassGraphWalker {
protected:
	size_t  _M_depth = 0;
	size_t  idx = 0;
	ColorID color_id = 0;
	valvec<VertexID> q1, q2;
	valvec< ColorID> color;
public:
	void resize(size_t VertexNum) {
		assert(q1.size() == idx);
		assert(q2.empty());
		q1.reserve(std::min<size_t>(VertexNum, 512));
		q2.reserve(std::min<size_t>(VertexNum, 512));
		q1.erase_all();
		idx = 0;
		_M_depth = 0;
		color.resize(VertexNum);
		color_id++;
	}
	void put_root(VertexID root) {
		assert(root < color.size());
		color[root] = color_id;
		q1.push_back(root);
	}
	VertexID next() {
		assert(idx <= q1.size());
		if (q1.size() == idx) {
			q1.swap(q2);
			idx = 0;
			_M_depth++;
			q2.erase_all();
		}
		assert(!q1.empty());
		return q1[idx++];
	}
	template<class Graph>
	void putChildren(const Graph* g, VertexID parent) {
		assert(parent < GraphTotalVertexes(g));
		g->for_each_dest(parent,
			[this,g](VertexID, VertexID child) {
				assert(child < GraphTotalVertexes(g));
				ASSERT_isNotFree2(g, child);
				assert(this->color[child] <= color_id);
				if (this->color[child] < color_id) {
					this->color[child] = color_id;
					this->q2.push_back(child);
				}
			});
	}
	template<class Graph, class OnEdge>
	void putChildren2(const Graph* g, VertexID parent, OnEdge on_edge) {
		assert(parent < GraphTotalVertexes(g));
		g->for_each_dest(parent,
			[&](VertexID parent, VertexID child) {
				assert(child < GraphTotalVertexes(g));
				ASSERT_isNotFree2(g, child);
				assert(this->color[child] <= color_id);
				if (this->color[child] < color_id) {
					if (on_edge(parent, child, true)) {
						this->color[child] = color_id;
						this->q2.push_back(child);
					}
				} else
					on_edge(parent, child, false);
			});
	}
	bool is_finished() const { return q1.size() + q2.size() == idx; }
	size_t depth() const { return _M_depth; }
};

template<class VertexID>
class BFS_TreeWalker {
protected:
	size_t _M_depth = 0;
	size_t idx = 0;
	valvec<VertexID> q1, q2;
public:
	void resize(size_t VertexNum) {
		assert(q1.size() == idx);
		assert(q2.empty());
		q1.reserve(std::min<size_t>(VertexNum, 512));
		q2.reserve(std::min<size_t>(VertexNum, 512));
		q1.erase_all();
		idx = 0;
		_M_depth = 0;
	}
	void put_root(VertexID root) {
		q1.push_back(root);
	}
	VertexID next() {
		assert(idx <= q1.size());
		if (q1.size() == idx) {
			q1.swap(q2);
			idx = 0;
			_M_depth++;
			q2.erase_all();
		}
		assert(!q1.empty());
		return q1[idx++];
	}
	template<class Graph>
	void putChildren(const Graph* g, VertexID parent) {
		assert(parent < GraphTotalVertexes(g));
		g->for_each_dest(parent,
			[this,g](VertexID, VertexID child) {
				assert(child < GraphTotalVertexes(g));
				ASSERT_isNotFree2(g, child);
				this->q2.push_back(child);
			});
	}
	template<class Graph, class OnEdge>
	void putChildren2(const Graph* g, VertexID parent, OnEdge on_edge) {
		assert(parent < GraphTotalVertexes(g));
		g->for_each_dest(parent,
			[&](VertexID parent, VertexID child) {
				assert(child < GraphTotalVertexes(g));
				ASSERT_isNotFree2(g, child);
				if (on_edge(parent, child, true)) {
					this->q2.push_back(child);
				}
			});
	}
	bool is_finished() const { return q1.size() + q2.size() == idx; }
	size_t depth() const { return _M_depth; }
};


/// The Fastest Graph Walker: similar with DFS and BFS,
/// but neither of them!
/// PFS is: Performance First Search
template<class VertexID>
class PFS_GraphWalker {
protected:
	valvec<VertexID> stack;
	simplebitvec     color;
public:
	void resize(size_t VertexNum) {
		assert(stack.empty());
		color.resize_no_init(VertexNum);
		color.fill(0);
		stack.reserve(512);
	}
	void put_root(VertexID root) {
		assert(root < color.size());
		stack.push_back(root);
		color.set1(root);
	}
	VertexID next() {
		assert(!stack.empty());
		VertexID x = stack.back();
		stack.pop_back();
		return x;
	}
	template<class Graph>
	void putChildren(const Graph* g, VertexID parent) {
		assert(parent < GraphTotalVertexes(g));
		g->for_each_dest_rev(parent,
			[this,g](VertexID, VertexID child) {
				assert(child < GraphTotalVertexes(g));
				ASSERT_isNotFree2(g, child);
				if (this->color.is0(child)) {
					this->stack.push_back(child);
					this->color.set1(child);
				}
			});
	}
	template<class Graph, class OnEdge>
	void putChildren2(const Graph* g, VertexID parent, OnEdge on_edge) {
		assert(parent < GraphTotalVertexes(g));
		g->for_each_dest_rev(parent,
			[&](VertexID parent, VertexID child) {
				assert(child < GraphTotalVertexes(g));
				ASSERT_isNotFree2(g, child);
				if (this->color.is0(child)) {
					if (on_edge(parent, child, true)) {
						this->stack.push_back(child);
						this->color.set1(child);
					}
				} else
					on_edge(parent, child, false);
			});
	}
	bool is_finished() const { return stack.empty(); }
};

template<class VertexID>
class DFS_GraphWalker { // in DFS Preorder
protected:
	valvec<VertexID> stack;
	simplebitvec     color;
public:
	void resize(size_t VertexNum) {
		assert(stack.empty());
		color.resize_no_init(VertexNum);
		color.fill(0);
		stack.reserve(512);
	}
	void put_root(VertexID root) {
		assert(root < color.size());
		stack.push_back(root);
		// don't call color.set1(root)
	}
	VertexID next() {
		assert(!stack.empty());
		VertexID x = stack.back();
		assert(color.is0(x));
		color.set1(x);
		do stack.pop_back(); // skip all visited states in stack
		while (!stack.empty() && color.is1(stack.back()));
		return x;
	}
	template<class Graph>
	void putChildren(const Graph* g, VertexID parent) {
		assert(parent < GraphTotalVertexes(g));
		g->for_each_dest_rev(parent,
			[this,g](VertexID, VertexID child) {
				assert(child < GraphTotalVertexes(g));
				ASSERT_isNotFree2(g, child);
				if (this->color.is0(child)) {
					this->stack.push_back(child);
					// Don't call color.set1(child) here!
					// so there may have multiple copy of child in stack
				}
			});
	}
	template<class Graph, class OnEdge>
	void putChildren2(const Graph* g, VertexID parent, OnEdge on_edge) {
		assert(parent < GraphTotalVertexes(g));
		g->for_each_dest_rev(parent,
			[&](VertexID parent, VertexID child) {
				assert(child < GraphTotalVertexes(g));
				ASSERT_isNotFree2(g, child);
				if (this->color.is0(child)) {
					if (on_edge(parent, child, true)) {
						this->stack.push_back(child);
						// Don't call color.set1(child) here!
						// so there may have multiple copy of child in stack
					}
				} else
					on_edge(parent, child, false);
			});
	}
	bool is_finished() const { return stack.empty(); }
};

template<class VertexID, class ColorID = VertexID>
class DFS_MultiPassGraphWalker { // in DFS Preorder
protected:
	valvec<VertexID> stack;
	valvec< ColorID> color;
	ColorID          color_id = 0;
public:
	void resize(size_t VertexNum) {
		assert(stack.empty());
		stack.reserve(512);
		color.resize(VertexNum);
		color_id++;
		// don't set color[root] = color_id;
	}
	void put_root(VertexID root) {
		assert(root < color.size());
		stack.push_back(root);
		// don't call color.set1(root)
	}
	VertexID next() { // in DFS Preorder
		assert(!stack.empty());
		VertexID x = stack.back();
		assert(color[x] < color_id);
		color[x] = color_id;
		do stack.pop_back(); // skip all visited states in stack
		while (!stack.empty() && color[stack.back()] == color_id);
		return x;
	}
	template<class Graph>
	void putChildren(const Graph* g, VertexID parent) {
		assert(parent < GraphTotalVertexes(g));
		g->for_each_dest_rev(parent,
			[this,g](VertexID, VertexID child) {
				assert(child < GraphTotalVertexes(g));
				ASSERT_isNotFree2(g, child);
				if (this->color[child] < color_id) {
					this->stack.push_back(child);
					// Don't set color[child] = color_id here!
					// so there may have multiple copy of child in stack
				}
			});
	}
	template<class Graph, class OnEdge>
	void putChildren2(const Graph* g, VertexID parent, OnEdge on_edge) {
		assert(parent < GraphTotalVertexes(g));
		g->for_each_dest_rev(parent,
			[&](VertexID parent, VertexID child) {
				assert(child < GraphTotalVertexes(g));
				ASSERT_isNotFree2(g, child);
				if (this->color[child] < color_id) {
					if (on_edge(parent, child, true)) {
						this->stack.push_back(child);
						// Don't set color[child] = color_id here!
						// so there may have multiple copy of child in stack
					}
				} else
					on_edge(parent, child, false);
			});
	}
	bool is_finished() const { return stack.empty(); }
};

template<class VertexID>
class DFS_TreeWalker { // in DFS Preorder
protected:
	valvec<VertexID> stack;
public:
	void resize(size_t /*VertexNum*/) {
		stack.reserve(512);
	}
	void put_root(VertexID root) {
		stack.push_back(root);
	}
	VertexID next() {
		assert(!stack.empty());
		VertexID x = stack.back();
		stack.pop_back();
		return x;
	}
	template<class Graph>
	void putChildren(const Graph* g, VertexID parent) {
		assert(parent < GraphTotalVertexes(g));
		g->for_each_dest_rev(parent,
			[this,g](VertexID, VertexID child) {
				assert(child < GraphTotalVertexes(g));
				ASSERT_isNotFree2(g, child);
				this->stack.push_back(child);
			});
	}
	template<class Graph, class OnEdge>
	void putChildren2(const Graph* g, VertexID parent, OnEdge on_edge) {
		assert(parent < GraphTotalVertexes(g));
		g->for_each_dest_rev(parent,
			[&](VertexID parent, VertexID child) {
				assert(child < GraphTotalVertexes(g));
				ASSERT_isNotFree2(g, child);
				if (on_edge(parent, child, true)) {
					this->stack.push_back(child);
				}
			});
	}
	bool is_finished() const { return stack.empty(); }
};


#endif // __febird_automata_graph_walker_hpp__


