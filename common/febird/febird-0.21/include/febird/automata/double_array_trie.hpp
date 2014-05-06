#ifndef __febird_automata_double_array_trie_hpp__
#define __febird_automata_double_array_trie_hpp__

#include <febird/valvec.hpp>
#include <febird/util/throw.hpp>
#include "automata.hpp"
#include <limits.h>

struct DA_State8B {
	uint32_t m_child0; // aka base, the child state on transition: '\0'
	uint32_t m_parent; // aka check

	typedef uint32_t state_id_t;
	typedef uint32_t position_t;
	static const uint32_t  max_state = 0x7FFFFFFF;
	static const uint32_t nil_state = 0x7FFFFFFF;

	DA_State8B() {
		m_child0 = nil_state; // also clear term_bit
		m_parent = nil_state | 0x80000000; // also set free_bit
	}

	void set_term_bit() { m_child0 |= 0x80000000; }
	void set_free_bit() { m_parent |= 0x80000000; }
	void set_child0(state_id_t x) { m_child0 = (m_child0 & 0x80000000) | x; }
	void set_parent(state_id_t x) { assert(x < max_state); m_parent = x; } // clear free_bit together

	bool is_term() const { return 0 != (m_child0 & 0x80000000); }
	bool is_free() const { return 0 != (m_parent & 0x80000000); }
	uint32_t child0() const { /*assert(!is_free());*/ return m_child0 & 0x7FFFFFFF; }
	uint32_t parent() const { /*assert(!is_free());*/ return m_parent; }

	void set_next(state_id_t next) {
		m_child0 = next | 0x80000000;
	}
	void set_prev(state_id_t prev) {
		m_parent = prev | 0x80000000;
	}
	void set_link(state_id_t prev, state_id_t next) {
		m_parent = prev | 0x80000000;
		m_child0 = next | 0x80000000;
	}

	typedef state_id_t transition_t;
	static transition_t first_trans(state_id_t t) { return t; }
};

template<class State>
class DoubleArrayTrie : public DFA_Interface {
protected:
	valvec<State> states;
	size_t  zpath_states = 0;
	mutable size_t transition_num;
public:
	enum { sigma = 512 };
	typedef State state_t;
	typedef typename State::state_id_t state_id_t;
	typedef typename State::state_id_t transition_t;
	static const state_id_t max_state = State::max_state;
	static const state_id_t nil_state = State::nil_state;
	static const state_id_t freelist_head = 1;

	DoubleArrayTrie() {
		m_dyn_sigma = sigma;
		states.resize(1);
		transition_num = 0;
	}
	~DoubleArrayTrie() {
		if (this->mmap_base) {
			states.risk_release_ownership();
		}
	}

	size_t num_used_states() const { 
		size_t n_used_tran = total_transitions();
		return n_used_tran + 1;
	}
	size_t num_free_states() const { 
		return states.size() - num_used_states();
	}

	void finish_load_mmap(const DFA_MmapHeader*);
	void prepare_save_mmap(DFA_MmapHeader*, const void**) const;

	const valvec<State>& internal_get_states() const { return states; }

	state_id_t state_move(state_id_t curr, auchar_t ch) const {
		assert(ch < sigma);
		assert(curr < states.size());
		state_id_t next = states[curr].child0() + ch;
		// if free_bit is set, m_parent never equals to any valid state
		// use free_bit instead of busy_bit, omited 1 bit mask op
		if (states[next].parent() == curr)
			return next;
		else
			return nil_state;
	}

	size_t total_states() const { return states.size(); }
	size_t total_transitions() const {
		if (0 == transition_num) {
			for (size_t i = 1, n = states.size(); i < n; ++i)
				if (!states[i].is_free())
					transition_num++;
		}
	   	return transition_num;
   	}
	size_t mem_size() const { return sizeof(State) * states.size(); }

	bool is_term(state_id_t s) const { return states[s].is_term(); }
	bool is_free(state_id_t s) const { return states[s].is_free(); }
	bool is_pzip(state_id_t s) const { return false; }

	void resize_states(state_id_t new_size) {
		assert(new_size < max_state);
		states.resize(new_size);
	}

	void swap(DoubleArrayTrie& y) {
		assert(typeid(*this) == typeid(y));
		DFA_Interface::swap(y);
		states.swap(y.states);
	}

	/// this is much slower than Automata::for_each_move
	template<class OP>
	void for_each_move(state_id_t curr, OP op) const {
		assert(!states[curr].is_free());
		state_id_t child0 = states[curr].child0();
		for (int ch = 0; ch < sigma; ++ch) {
			state_id_t next = child0 + ch;
			if (states[next].parent() == curr)
				op(curr, next, (auchar_t)ch);
		}
	}
	template<class OP>
	void for_each_dest(state_id_t curr, OP op) const {
		assert(!states[curr].is_free());
		state_id_t child0 = states[curr].child0();
		for (int ch = 0; ch < sigma; ++ch) {
			state_id_t next = child0 + ch;
			if (states[next].parent() == curr)
				op(curr, next);
		}
	}
	template<class OP>
	void for_each_dest_rev(state_id_t curr, OP op) const {
		assert(!states[curr].is_free());
		state_id_t child0 = states[curr].child0();
		for (int ch = sigma-1; ch >= 0; --ch) {
			state_id_t next = child0 + ch;
			if (states[next].parent() == curr)
				op(curr, next);
		}
	}

	const byte_t* get_zpath_data(size_t) const {
		assert(0);
		abort();
		return NULL;
	}
	
	bool has_freelist() const override final { return true; }

	// requires DA_State::set_parent(state_id)
	//                    set_child0(state_id)
	//                    set_term_bit()
	template<class Y_AU>
	void build_from(const Y_AU& y_au
				  , valvec<typename Y_AU::state_id_t>& map_x2y
				  , valvec<               state_id_t>& map_y2x
				  , const char* WalkMethod
				  , size_t extra = 0)
	{
		typedef BFS_TreeWalker<typename Y_AU::state_id_t> BFS;
		typedef DFS_TreeWalker<typename Y_AU::state_id_t> DFS;
		if (strcasecmp(WalkMethod, "BFS") == 0) {
			build_from_aux<Y_AU, BFS>(y_au, map_x2y, map_y2x, extra);
		}
	   	else if (strcasecmp(WalkMethod, "DFS") == 0) {
			build_from_aux<Y_AU, DFS>(y_au, map_x2y, map_y2x, extra);
		}
		else {
			THROW_STD(invalid_argument, "WalkMethod=%s", WalkMethod);
		}
	}
private:
   	template<class Y_AU, class Y_Walker>
	void build_from_aux(const Y_AU& y_au
				  , valvec<typename Y_AU::state_id_t>& map_x2y
				  , valvec<               state_id_t>& map_y2x
				  , size_t extra)
	{
	//	build_from_aux_freelist(y_au, map_x2y, map_y2x, extra);
		build_from_aux_huristic<Y_AU, Y_Walker>(y_au, map_x2y, map_y2x, extra);
#if 0
		for (size_t i = 0; i < states.size()-1; i++) {
			printf("states[%03zd]=(%d %03u %03u))\n", i, states[i].is_term(), states[i].child0(), states[i].check);
		}
#endif
	}

	template<class Y_AU, class Y_Walker>
	void build_from_aux_huristic(const Y_AU& y_au
				  , valvec<typename Y_AU::state_id_t>& map_x2y
				  , valvec<               state_id_t>& map_y2x
				  , size_t extra)
	{
		typedef typename Y_AU::state_id_t Ystate_id_t;
		const auto& y_states = y_au.internal_get_states();
		assert(y_states.size() >= 1);
		states .resize(0);
		map_x2y.resize(0);
		map_y2x.resize(0);
		states .resize(y_states.size()*3/2);
		map_x2y.resize(y_states.size()*3/2, (Ystate_id_t)Y_AU::nil_state);
		map_y2x.resize(y_states.size()    , ( state_id_t)this->nil_state);
		map_y2x[initial_state] = initial_state;
		map_x2y[initial_state] = initial_state;
		valvec<CharTarget<Ystate_id_t> > labels;
	   	labels.reserve(sigma);
		Y_Walker walker;
		walker.resize(y_states.size());
		walker.put_root(initial_state);
		size_t curr_slot = 1;
		size_t real_size = 1;
		while (!walker.is_finished()) {
			const Ystate_id_t y_parent = walker.next();
			labels.resize0();
			y_au.for_each_move(y_parent, [&](Ystate_id_t,Ystate_id_t y_child, auchar_t ch){
				assert(ch < sigma);
				labels.unchecked_push_back({ch, y_child});
			});
			size_t prev_slot = curr_slot;
		Retry:
			for (size_t i = 0, n = labels.size(); i < n; ++i) {
				auto     ch    = labels[i].ch;
				size_t x_child = curr_slot + ch;
				if (x_child >= states.size()) {
					states .resize(x_child*3/2);
					map_x2y.resize(x_child*3/2, (Ystate_id_t)Y_AU::nil_state);
				}
				else if (!states[x_child].is_free()) {
					do {
						if (++x_child == states.size()) {
							curr_slot =  states.size();
							goto Retry;
						}
					} while (!states[x_child].is_free());
					curr_slot = x_child - ch;
					if (1 == n)	break;
					else goto Retry;
				}
			}
			size_t x_parent = map_y2x[y_parent];
			states[x_parent].set_child0(curr_slot);
			for (size_t i = 0, n = labels.size(); i < n; ++i) {
				Ystate_id_t y_child = labels[i].target;
				auto          ch    = labels[i].ch;
				size_t      x_child = curr_slot + ch;
			//	assert(nil_state == map_y2x[y_child]);
				assert(nil_state != map_y2x[y_parent]);
				map_y2x[y_child] = x_child;
				map_x2y[x_child] = y_child;
				states[x_child].set_parent(x_parent);
				if (y_states[y_child].is_term())
					states[x_child].set_term_bit();
				if (x_child >= real_size)
					real_size = x_child + 1;
			}
			walker.putChildren(&y_au, y_parent);
			size_t huristic_factor = 16;
			curr_slot = prev_slot + (curr_slot - prev_slot)/huristic_factor;
		}
		map_x2y.resize(real_size + extra + sigma, (Ystate_id_t)Y_AU::nil_state);
		map_x2y.shrink_to_fit();
		states .resize(real_size + extra + sigma);
		states .shrink_to_fit();
		for (size_t i = 0; i < real_size; i++) {
			if (states[i].child0() == nil_state)
				states[i].set_child0(1);
		}
		states[initial_state].set_parent(initial_state);
	}

	void init_freelist() {

	}

	void use_slot(size_t slot) {
		state_id_t my_next = states[slot].get_next();
		state_id_t my_prev = states[slot].get_prev();
		states[my_prev].set_next(my_next);
		states[my_next].set_prev(my_prev);
	}

	void states_grow_to(size_t newsize) {
		size_t oldsize = states.size();
		assert(newsize > oldsize);
		states.resize_no_init(newsize);
		for (size_t i = oldsize; i < newsize-1; ++i) {
			states[i].set_link(i, i+1);
		}
		states.back().set_link(newsize-2, 1); // prev=newsize-2, next=1
	}

	template<class DataIO>
	void load_trie(DataIO& dio) {
		uint64_t ui64;
		dio >> ui64; this->transition_num = ui64;
		dio >> ui64;
		this->states.resize(ui64);
		dio.ensureRead(states.data(), sizeof(State) * ui64);
	}
	template<class DataIO>
	void save_trie(DataIO& dio) const {
		dio << uint64_t(this->transition_num);
		dio << uint64_t(this->states.size());
		dio.ensureWrite(states.data(), sizeof(State) * states.size());
	}

	enum { SERIALIZATION_VERSION = 1 };

	template<class DataIO>
	friend void DataIO_loadObject(DataIO& dio, DoubleArrayTrie& self) {
		typename DataIO::my_var_uint64_t version;
		dio >> version;
		if (version > SERIALIZATION_VERSION) {
			typedef typename DataIO::my_BadVersionException bad_ver;
			throw bad_ver(version.t, SERIALIZATION_VERSION, "DoubleArrayTrie");
		}
		self.load_trie(dio);
	}
	template<class DataIO>
	friend void DataIO_saveObject(DataIO& dio, const DoubleArrayTrie& self) {
		dio << typename DataIO::my_var_uint64_t(SERIALIZATION_VERSION);
		self.save_trie(dio);
	}
typedef DoubleArrayTrie MyType;
#include "ppi/dfa_match.hpp"
#include "ppi/dfa_const_virtuals.hpp"
};

typedef DoubleArrayTrie<DA_State8B> DoubleArrayTrie_8B;

#endif // __febird_automata_double_array_trie_hpp__

