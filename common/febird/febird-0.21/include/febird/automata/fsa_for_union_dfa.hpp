#ifndef __febird_automata_fast_union_dfa_hpp__
#define __febird_automata_fast_union_dfa_hpp__

#include "dfa_interface.hpp"
#include <febird/util/throw.hpp>
#include <febird/gold_hash_map.hpp>
#include <boost/noncopyable.hpp>

template<class DFA>
class NFA_ForMultiRootDFA : boost::noncopyable {
	const DFA* dfa;
	valvec<size_t> roots;
public:
	enum { sigma = DFA::sigma };
	typedef typename DFA::state_id_t state_id_t;
	static const state_id_t nil_state = DFA::nil_state;
	static const state_id_t max_state = DFA::max_state;

	explicit NFA_ForMultiRootDFA(const DFA* dfa1) : dfa(dfa1) {}
	size_t get_sigma() const { return sigma; }
	size_t total_states() const { return dfa->total_states(); }

	bool is_final(size_t s) const {
		assert(s < dfa->total_states());
		return dfa->is_term(s);
	}
	void get_epsilon_targets(size_t s, valvec<state_id_t>* children) const {
		assert(s < dfa->total_states());
		children->erase_all();
		if (initial_state == s) {
			for (size_t root : roots)
				children->push_back(root);
		}
	}
	void get_non_epsilon_targets(size_t s, valvec<CharTarget<size_t> >* children)
	const {
		assert(s < dfa->total_states());
		dfa->get_all_move(s, children);
	}
	void add_root(size_t root) { roots.push_back(root); }
};

// When treated as an NFA, only initial_state is nondeterminized
//   1. NFA_to_DFA_make_dfa can be applied on this "NFA" to get a unioned
//      DFA of all sub DFAs
//
// When treated as an DFA, this DFA has multiple individule sub DFAs
//   1. This "DFA" can be minimized by general DFA minimization algorithms,
//      such as Hopcroft, Peng Lei's DFA_Algorithm::adfa_minimize
//   2. In general, DFA minimization algorithm could only merge tails of
//      multiple sub DFAs
//   3. If sub DFA1 is equivalent to sub DFA2, after adfa_minimize, the root2
//      will have two identical elements corresponding to DFA1 and DFA2
//   4. Calling NFA_to_DFA_make_dfa after adfa_minimize to create the union-dfa
//      will significantly reduce memory and running time
class FSA_ForUnionDFA : boost::noncopyable {
	mutable valvec<size_t> m_stack_dests;
	mutable valvec<CharTarget<size_t> > m_stack_moves;
	valvec<uint32_t> m_index;
	valvec<const DFA_Interface*> m_dfas;
	size_t m_total_states;
	size_t m_max_sigma;
	size_t m_free_states;
	bool   m_is_owner;

public:
	typedef uint32_t state_id_t;
	enum { sigma = 512 };
	static const state_id_t nil_state = uint32_t(-1);
	static const state_id_t max_state = uint32_t(-1);

	FSA_ForUnionDFA();
	~FSA_ForUnionDFA();

	void clear();
	void set_own(bool v) { m_is_owner = v; }
	void add_dfa(const DFA_Interface* dfa);
	void get_all_root(valvec<state_id_t>* roots) const;
	void compile();

	// NFA methods:
	size_t get_sigma() const { return m_max_sigma; }
	size_t total_states() const { return m_total_states; }
	bool is_final(size_t s) const;
	void get_epsilon_targets(size_t s, valvec<state_id_t>* children) const;
	void get_non_epsilon_targets(size_t s, valvec<CharTarget<size_t> >* children) const;

	// DFA methods: the FSA is DFA except initial_state
	// the DFA has multiple roots, and can be minimized by Hopcroft/adfa_minimize
	//
	size_t get_all_dest(size_t s, size_t* dests) const;
	size_t get_all_move(size_t s, CharTarget<size_t>* moves) const;

	template<class OP>
	void for_each_dest(size_t s, OP op) const {
		size_t cnt;
		size_t oldsize = risk_get_all_dest(s, &cnt);
		for (size_t j = oldsize; j < oldsize + cnt; ++j) {
			op(s, m_stack_dests[j]);
		}
		m_stack_dests.risk_set_size(oldsize);
	}

	template<class OP>
	void for_each_dest_rev(size_t s, OP op) const {
		size_t cnt;
		size_t oldsize = risk_get_all_dest(s, &cnt);
		for (size_t j = oldsize + cnt; j > oldsize; ) {
			--j;
			op(s, m_stack_dests[j]);
		}
		m_stack_dests.risk_set_size(oldsize);
	}

	template<class OP>
	void for_each_move(size_t s, OP op) const {
		size_t cnt;
		size_t oldsize = risk_get_all_move(s, &cnt);
		for (size_t i = oldsize; i < oldsize + cnt; ++i) {
			CharTarget<size_t> ct = m_stack_moves[i];
			op(s, ct.target, ct.ch);
		}
		m_stack_moves.risk_set_size(oldsize);
	}

	size_t risk_get_all_dest(size_t s, size_t* pcnt) const;
	size_t risk_get_all_move(size_t s, size_t* pcnt) const;

	size_t num_zpath_states() const { return 0; }
	bool is_dag() const;
	bool is_free(size_t s) const; 
	bool is_term(size_t s) const { return is_final(s); }
	bool is_pzip(size_t  ) const { return false; }

	size_t num_used_states() const { return m_total_states - m_free_states; }
	size_t num_free_states() const { return m_free_states; }
	const byte_t* get_zpath_data(size_t) const { return NULL; }

	// for adfa_minimize
	size_t adfa_hash_hash(const state_id_t* Min, size_t state_id) const;
	bool   adfa_hash_equal(const state_id_t* Min, size_t x, size_t y) const;

typedef FSA_ForUnionDFA MyType;
#include "ppi/post_order.hpp"
#include "ppi/adfa_minimize.hpp"
};

#endif // __febird_automata_fast_union_dfa_hpp__

