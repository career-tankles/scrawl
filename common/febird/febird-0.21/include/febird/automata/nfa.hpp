#ifndef __febird_nfa_hpp__
#define __febird_nfa_hpp__

#include <febird/config.hpp>
#include <febird/fstring.hpp>
#include <febird/gold_hash_map.hpp>
#include <febird/valvec.hpp>
#include <febird/bitmap.hpp>
#include <febird/smallmap.hpp>
#include <stdio.h>
#include <errno.h>
#include "dfa_interface.hpp"
#include "graph_walker.hpp"

struct NFA_Transition4B {
	NFA_Transition4B(uint32_t target, auchar_t ch) {
		this->target = target;
		this->ch     = ch;
	}
	uint32_t target : 23;
	unsigned ch     :  9;
	typedef uint32_t state_id_t;
	enum { sigma = 512 };
};

template<class Transition = NFA_Transition4B>
class GenericNFA {
public:
	typedef typename Transition::state_id_t state_id_t;
protected:
	struct CharLess {
		bool operator()(const Transition& x, state_id_t y) const { return x.ch < y; }
		bool operator()(state_id_t x, const Transition& y) const { return x < y.ch; }
	};
	struct NfaState {
		valvec<Transition> targets;
	   	// first n_epsilon elements are epsilon transitions
		// 'ch' of epsilon transtions are ignored
		size_t    n_epsilon : 8*sizeof(size_t)-1;
		unsigned  b_final   : 1;
		NfaState() { n_epsilon = 0; b_final = 0; }
	};
	valvec<NfaState> states;

public:
	typedef std::pair<const Transition*, const Transition*> target_range_t;
	typedef std::pair<      Transition*,       Transition*> target_range_tm; // mutable
	enum { sigma = Transition::sigma };

	GenericNFA() {
		states.resize(1); // make initial_state
	}

	size_t get_sigma() const { return sigma; }

	size_t total_states() const { return states.size(); }

	state_id_t new_state() {
	   	state_id_t s = states.size();
	   	states.push_back();
	   	return s;
   	}

	// if trans has extra complex members, use this function
	void add_move(state_id_t source, const Transition& trans) {
		assert(source       < states.size());
		assert(trans.target < states.size());
		valvec<Transition>& t = states[source].targets;
		auto pos = std::upper_bound(t.begin(), t.end(), trans.ch, CharLess());
		t.insert(pos, trans);
	}
	void add_move(state_id_t source, state_id_t target, auchar_t ch) {
		assert(source < states.size());
		assert(target < states.size());
		valvec<Transition>& t = states[source].targets;
		auto pos = std::upper_bound(t.begin(), t.end(), ch, CharLess());
		t.insert(pos, Transition(target, ch));
	}

	void add_epsilon(state_id_t source, const Transition& trans) {
		assert(source       < states.size());
		assert(trans.target < states.size());
		NfaState& s = states[source];
		s.targets.insert(s.n_epsilon, trans);
		s.n_epsilon++;
	}
	void add_epsilon(state_id_t source, state_id_t target) {
		assert(source < states.size());
		assert(target < states.size());
		NfaState& s = states[source];
		s.targets.insert(s.n_epsilon, Transition(target, 0));
		s.n_epsilon++;
	}

	bool is_final(state_id_t state) const {
		assert(state < states.size());
		return states[state].b_final;
	}

	void set_final(state_id_t state) {
		assert(state < states.size());
		states[state].b_final = 1;
	}

	void get_epsilon_targets(state_id_t s, valvec<state_id_t>* children) const {
		assert(s < states.size());
		const NfaState& ts = states[s];
		children->erase_all();
		children->reserve(ts.n_epsilon);
		for (size_t i = 0; i < ts.n_epsilon; ++i)
			children->unchecked_push_back(ts.targets[i].target);
	}
	void get_non_epsilon_targets(state_id_t s, valvec<CharTarget<size_t> >* children) const {
		assert(s < states.size());
		const NfaState& ts = states[s];
		children->erase_all();
		children->reserve(ts.targets.size() - ts.n_epsilon);
		for (size_t i = ts.n_epsilon; i < ts.targets.size(); ++i)
			children->unchecked_push_back(
				CharTarget<size_t>(ts.targets[i].ch, ts.targets[i].target));
	}
};

// a NFA which is derived from a DFA, there are few states of the NFA
// are Non-Determinal, most states are determinal
// Non-Determinal states are save in a hash map
template<class DFA>
class NFA_BaseOnDFA {
public:
	typedef typename DFA::state_id_t state_id_t;
	enum { sigma = DFA::sigma };
private:
	DFA* dfa;
	gold_hash_map<state_id_t, valvec<state_id_t> > epsilon;
	gold_hash_map<state_id_t, valvec<CharTarget<state_id_t> > > non_epsilon;

public:
	NFA_BaseOnDFA(DFA* dfa1) : dfa(dfa1) {}

	size_t get_sigma() const { return sigma; }

	void reset(DFA* dfa1) {
	   	dfa = dfa1;
		this->erase_all();
	}
	void erase_all() {
	   	epsilon.erase_all();
	   	non_epsilon.erase_all();
   	}
	void resize_states(size_t n) { dfa->resize_states(n); }

	size_t total_states() const { return dfa->total_states(); }

	state_id_t new_state() { return dfa->new_state(); }

	/// will not add duplicated (targets, ch)
	/// @param ch the transition label char
	void add_move(state_id_t source, state_id_t target, state_id_t ch) {
		assert(ch < sigma);
		state_id_t existed_target = dfa->add_move(source, target, ch);
		if (DFA::nil_state == existed_target || target == existed_target)
			return; // Good luck, it is determinal
		auto& targets = non_epsilon[source];
		CharTarget<state_id_t> char_target(ch, target);
		auto  ins_pos = std::lower_bound(targets.begin(), targets.end(), char_target);
		if (targets.end() == ins_pos || char_target < *ins_pos)
			targets.insert(ins_pos, char_target);
	}
	void push_move(state_id_t source, state_id_t target, state_id_t ch) {
		assert(ch < sigma);
		state_id_t existed_target = dfa->add_move(source, target, ch);
		if (DFA::nil_state == existed_target || target == existed_target)
			return; // Good luck, it is determinal
		auto& targets = non_epsilon[source];
		targets.push_back(CharTarget<state_id_t>(ch, target));
	}

	/// will not add duplicated targets
	void add_epsilon(state_id_t source, state_id_t target) {
		auto& eps = epsilon[source];
		auto  ins_pos = std::lower_bound(eps.begin(), eps.end(), target);
		if (eps.end() == ins_pos || target < *ins_pos) {
		//	fprintf(stderr, "add_epsilon: source=%u target=%u\n", source, target);
			eps.insert(ins_pos, target);
		}
	}

	/// may add duplicated targets
	void push_epsilon(state_id_t source, state_id_t target) {
		epsilon[source].push_back(target);
	}

	void sort_unique_shrink_all() {
		assert(!epsilon.freelist_is_using());
		assert(!non_epsilon.freelist_is_using());
		for (size_t i = 0; i < epsilon.end_i(); ++i) {
		//	assert(!epsilon.freelist_is_using() || !epsilon.is_deleted(i));
			auto& eps = epsilon.val(i);
			std::sort(eps.begin(), eps.end());
			eps.trim(std::unique(eps.begin(), eps.end()));
			eps.shrink_to_fit();
		}
		for (size_t i = 0; i < non_epsilon.end_i(); ++i) {
			auto& tar = non_epsilon.val(i);
			std::sort(tar.begin(), tar.end());
			tar.trim(std::unique(tar.begin(), tar.end()));
			tar.shrink_to_fit();
		}
		epsilon.shrink_to_fit();
		non_epsilon.shrink_to_fit();
		dfa->shrink_to_fit();
	}

	bool is_final(state_id_t s) const {
		assert(s < dfa->total_states());
		return dfa->is_term(s);
	}
	void set_final(state_id_t s) {
		assert(s < dfa->total_states());
		dfa->set_term_bit(s);
	}

	void get_epsilon_targets(state_id_t s, valvec<state_id_t>* children) const {
		assert(s < dfa->total_states());
		size_t idx = epsilon.find_i(s);
		if (epsilon.end_i() == idx) {
			children->erase_all();
		} else {
			auto& eps = epsilon.val(idx);
			children->assign(eps.begin(), eps.end());
		//	fprintf(stderr, "get_epsilon_targets(%u):", s);
		//	for (state_id_t t : epsilon) fprintf(stderr, " %u", t);
		//	fprintf(stderr, "\n");
		}
	}
	void get_non_epsilon_targets(state_id_t s, valvec<CharTarget<size_t> >* children) const {
		assert(s < dfa->total_states());
		size_t idx = non_epsilon.find_i(s);
		dfa->get_all_move(s, children);
		if (non_epsilon.end_i() != idx) {
			auto& targets = non_epsilon.val(idx);
			size_t n_xxx = targets.size();
			size_t n_dfa = children->size();
			children->resize_no_init(n_dfa + n_xxx);
			auto src = targets.begin();
			auto dst = children->begin() + n_dfa;
			for (size_t i = 0; i < n_xxx; ++i) {
				auto& x = dst[i];
				auto& y = src[i];
				x.ch = y.ch;
				x.target = y.target;
			}
		}
	}

	void concat(state_id_t root1, state_id_t root2) {
		PFS_GraphWalker<state_id_t> walker;
		walker.resize(dfa->total_states());
		walker.put_root(root1);
		while (!walker.is_finished()) {
			state_id_t curr = walker.next();
			if (dfa->is_term(curr)) {
				dfa->clear_term_bit(curr);
				add_epsilon(curr, root2);
			}
			walker.putChildren(dfa, curr);
		}
	}

	void suffix_dfa_add_epsilon_on_utf8() {
		valvec<state_id_t> stack;
		simplebitvec color(dfa->total_states());
		stack.push_back(initial_state);
		color.set1(initial_state);
		while (!stack.empty()) {
			state_id_t parent = stack.back(); stack.pop_back();
			bool has_utf8_head = false;
			dfa->for_each_move(parent,
				[&](state_id_t, state_id_t child, auchar_t ch) {
					if ((ch & 0x80) == 0 || (ch & 0xC0) == 0xC0) {
						has_utf8_head = true;
					}
					if (color.is0(child)) {
						color.set1(child);
						stack.push_back(child);
					}
				});
			if (initial_state != parent && has_utf8_head) {
				push_epsilon(initial_state, parent);
			}
		}
		sort_unique_shrink_all();
	}

	void print_nfa_info(FILE* fp, const char* indent_word = "") const {
		assert(NULL != indent_word);
		assert(!epsilon.freelist_is_using());
		assert(!non_epsilon.freelist_is_using());
		size_t node_eps = 0, node_non_eps = 0;
		size_t edge_eps = 0, edge_non_eps = 0;
		for(size_t i = 0; i < epsilon.end_i(); ++i) {
			auto&  x = epsilon.val(i);
			node_eps += x.size() ? 1 : 0;
			edge_eps += x.size();
		}
		for(size_t i = 0; i < non_epsilon.end_i(); ++i) {
			auto&  x = non_epsilon.val(i);
			node_non_eps += x.size() ? 1 : 0;
			edge_non_eps += x.size();
		}
		fprintf(fp, "%snumber of states which has ____epsilon: %zd\n", indent_word, node_eps);
		fprintf(fp, "%snumber of states which has non_epsilon: %zd\n", indent_word, node_non_eps);
		fprintf(fp, "%snumber of all ____epsilon transitions: %zd\n", indent_word, edge_eps);
		fprintf(fp, "%snumber of all non_epsilon transitions: %zd\n", indent_word, edge_non_eps);
	}
};

template<class StateID>
struct CompactReverseNFA {
	valvec<StateID> index; // parallel with Automata::states
	valvec<CharTarget<StateID> > data;
	gold_hash_map<StateID, valvec<StateID> > epsilon;
	simplebitvec final_bits;

	enum { sigma = 512 };
	size_t get_sigma() const { return sigma; }

	typedef StateID state_id_t;

	void clear() {
		index.clear();
		data.clear();
		final_bits.clear();
		epsilon.clear();
	}

	void erase_all() {
		index.erase_all();
		data.erase_all();
		final_bits.erase_all();
		epsilon.erase_all();
	}

	void set_final(size_t s) {
		assert(s < index.size()-1);
		if (final_bits.size() <= s) {
			final_bits.resize(s+1);
		}
		final_bits.set1(s);
	}
	bool is_final(size_t s) const {
		assert(s < index.size()-1);
		if (s >= final_bits.size())
			return false;
		else
			return final_bits.is1(s);
	}
	size_t total_states() const { return index.size()-1; }

	/// will not add duplicated targets
	void add_epsilon(StateID source, StateID target) {
		auto& eps = epsilon[source];
		auto  ins_pos = std::lower_bound(eps.begin(), eps.end(), target);
		if (eps.end() == ins_pos || target < *ins_pos) {
		//	fprintf(stderr, "add_epsilon: source=%u target=%u\n", source, target);
			eps.insert(ins_pos, target);
		}
	}

	/// may add duplicated targets
	void push_epsilon(StateID source, StateID target) {
		epsilon[source].push_back(target);
	}

	void sort_unique_shrink_all() {
		assert(!epsilon.freelist_is_using());
		for(size_t i = 0; i < epsilon.end_i(); ++i) {
		//	assert(!epsilon.freelist_is_using() || !epsilon.is_deleted(i));
			auto& eps = epsilon.val(i);
			std::sort(eps.begin(), eps.end());
			eps.trim(std::unique(eps.begin(), eps.end()));
			eps.shrink_to_fit();
		}
		epsilon.shrink_to_fit();
		for(size_t i = 0; i < index.size()-1; ++i) {
			size_t lo = index[i+0];
			size_t hi = index[i+1];
			std::sort(data.begin()+lo, data.begin()+hi);
		}
		index.shrink_to_fit();
		data.shrink_to_fit();
	}

	void get_epsilon_targets(StateID s, valvec<StateID>* children) const {
		assert(s < index.size()-1);
		children->erase_all();
		size_t idx = epsilon.find_i(s);
		if (epsilon.end_i() != idx) {
			auto& eps = epsilon.val(idx);
			children->assign(eps.begin(), eps.end());
		//	fprintf(stderr, "get_epsilon_targets(%u):", s);
		//	for (StateID t : eps) fprintf(stderr, " %u", t);
		//	fprintf(stderr, "\n");
		}
	}
	void get_non_epsilon_targets(StateID s, valvec<CharTarget<size_t> >* children) const {
		assert(s < index.size()-1);
		children->erase_all();
		size_t lo = index[s+0];
		size_t hi = index[s+1];
		children->assign(data.begin()+lo, hi-lo);
	}

	void print_nfa_info(FILE* fp, const char* indent_word = "") const {
		assert(!epsilon.freelist_is_using());
		assert(NULL != indent_word);
		size_t edge_eps = 0;
		for(size_t i = 0; i < epsilon.end_i(); ++i) {
			edge_eps += epsilon.val(i).size();
		}
		fprintf(fp, "%snumber of states which has epsilon: %zd\n", indent_word, epsilon.size());
		fprintf(fp, "%snumber of all epsilon transitions: %zd\n", indent_word, edge_eps);
	}
};

struct powerkey_t {
	size_t offset;
	size_t length;

	powerkey_t(size_t offset1, size_t length1)
	   	: offset(offset1), length(length1) {}
	powerkey_t() : offset(), length() {}
};

template<class PowerIndex>
struct PowerSetKeyExtractor {
	powerkey_t operator()(const PowerIndex& x0) const {
		// gold_hash is ValueInline, and Data==Link==PowerIndex
		// so next Index is at (&x0)[2], (&x0)[1] is hash link
		PowerIndex x1 = (&x0)[2];
	//	static long n = 0;
	//	printf("n=%ld x0=%ld x1=%ld\n", n++, (long)x0, (long)x1);
		return powerkey_t(x0, x1-x0);
	}
};

template<class StateID>
struct PowerSetHashEq {
	bool equal(powerkey_t x, powerkey_t y) const {
		assert(x.length > 0);
		assert(x.offset + x.length <= power_set.size());
		assert(y.length > 0);
		assert(y.offset + y.length <= power_set.size());
		if (x.length != y.length) {
			return false;
		}
		const StateID* p = &power_set[0];
		size_t x0 = x.offset;
		size_t y0 = y.offset;
		size_t nn = x.length;
		do {
			if (p[x0] != p[y0])
				return false;
			++x0, ++y0;
		} while (--nn);
		return true;
	}
	size_t hash(powerkey_t x) const {
		assert(x.length > 0);
		assert(x.offset + x.length <= power_set.size());
		const StateID* p = &power_set[x.offset];
		size_t h = 0;
		size_t n = x.length;
		do {
			h += *p++;
			h *= 31;
			h += h << 3 | h >> (sizeof(h)*8-3);
		} while (--n);
		return h;
	}
	explicit PowerSetHashEq(const valvec<StateID>& power_set)
	 : power_set(power_set) {}

	const valvec<StateID>& power_set;
};

template<class StateID, class PowerIndex>
struct PowerSetMap {
	// must be FastCopy/realloc
	typedef node_layout<PowerIndex, PowerIndex, FastCopy, ValueInline> Layout;
	typedef PowerSetHashEq<StateID> HashEq;
	typedef PowerSetKeyExtractor<PowerIndex> KeyExtractor;
	typedef gold_hash_tab<powerkey_t, PowerIndex
				, HashEq, KeyExtractor, Layout
				>
			base_type;
	struct type : base_type {
		type(valvec<StateID>& power_set)
		  : base_type(64, HashEq(power_set), KeyExtractor())
		{}
	};
};

template<class NFA>
class NFA_to_DFA {
	NFA* nfa;

public:
	NFA_to_DFA(NFA* nfa1) : nfa(nfa1) {}

	typedef typename NFA::state_id_t  state_id_t;

	class SinglePassPFS_Walker {
		const NFA*         nfa;
		simplebitvec       color;
		valvec<state_id_t> stack;
		valvec<CharTarget<size_t> >  cache_non_epsilon;
		valvec<state_id_t>  cache_epsilon;

	public:
		SinglePassPFS_Walker(const NFA* nfa1) {
			nfa = nfa1;
			color.resize(nfa1->total_states(), 0);
		}
		void clear_color() {
			color.fill(0);
		}
		void put_root(state_id_t root) {
			assert(root < nfa->total_states());
			assert(color.is0(root));
			color.set1(root);
			stack.push_back(root);
		}
		void discover_epsilon_children(state_id_t curr) {
			nfa->get_epsilon_targets(curr, &cache_epsilon);
			// push to stack in reverse order
			for (size_t i = cache_epsilon.size(); i > 0; --i) {
				state_id_t  target = cache_epsilon[i-1];
				if (color.is0(target)) {
					color.set1(target);
					stack.push_back(target);
				}
			}
			cache_epsilon.erase_all();
		}
		void discover_non_epsilon_children(state_id_t curr) {
			nfa->get_non_epsilon_targets(curr, &cache_non_epsilon);
			// push to stack in reverse order
			for (size_t i = cache_non_epsilon.size(); i > 0; --i) {
				state_id_t  target = cache_non_epsilon[i-1].target;
			#if !defined(NDEBUG) 
				auchar_t ch = cache_non_epsilon[i-1].ch;
				assert(ch < nfa->get_sigma());
			#endif
				if (color.is0(target)) {
					color.set1(target);
					stack.push_back(target);
				}
			}
			cache_non_epsilon.erase_all();
		}
		state_id_t next() {
		   	assert(!stack.empty());
			state_id_t s = stack.back();
			stack.unchecked_pop_back();
		   	return s;
	   	}
		bool is_finished() const { return stack.empty(); }

		// PFS search for epsilon closure
		void gen_epsilon_closure(state_id_t source, valvec<state_id_t>* closure) {
			put_root(source);
			size_t closure_oldsize = closure->size();
			while (!is_finished()) {
				state_id_t curr = next();
				closure->push_back(curr);
				discover_epsilon_children(curr);
			}
			// reset color of touched states backward:
			//   the last color[ps[*]] are more recently accessed
			//   and more likely in CPU cache
			const auto  pc = color.bldata();
			const auto  cbits = sizeof(pc[0]) * 8;
			const auto  ns = closure->size() - closure_oldsize;
			const auto  ps = closure->data() + closure_oldsize - 1;
			for(auto i = ns; i > 0; --i) pc[ps[i]/cbits] = 0;
		}
	};

	struct NFA_SubSet : valvec<state_id_t> {
		void resize0() {
            this->ch = -1;
            valvec<state_id_t>::resize0();
		}
		NFA_SubSet() { ch = -1; }
		ptrdiff_t  ch; // use ptrdiff_t for better align
	};

	/// @param out dfa_states.back() is the guard
	/// @param out power_set[dfa_states[i]..dfa_states[i+1]) is the nfa subset for dfa_state 'i'
	/// @param out dfa
	/// PowerIndex integer type: (uint32_t or uint64_t)
	template<class PowerIndex, class Dfa>
	size_t make_dfa(valvec<PowerIndex>* dfa_states, valvec<state_id_t>& power_set
			    , Dfa& dfa, size_t max_power_set = ULONG_MAX) const {
		return s_make_dfa(nfa, dfa_states, power_set, dfa, max_power_set);
	}
	template<class PowerIndex, class Dfa>
	static
	size_t s_make_dfa(const NFA* nfa
				  , valvec<PowerIndex>* dfa_states, valvec<state_id_t>& power_set
			      , Dfa& dfa, size_t max_power_set = ULONG_MAX) {
	//	BOOST_STATIC_ASSERT(Dfa::sigma >= NFA::sigma);
		dfa.erase_all();
		power_set.erase_all();
		typename PowerSetMap<state_id_t, PowerIndex>::type power_set_map(power_set);
		smallmap<NFA_SubSet> dfa_next(nfa->get_sigma()); // collect the targets of a dfa state
		SinglePassPFS_Walker walker(nfa);
		valvec<CharTarget<size_t> > children;
		walker.gen_epsilon_closure(initial_state, &power_set);
		std::sort(power_set.begin(), power_set.end());
	//	power_set.trim(std::unique(power_set.begin(), power_set.end()));
		assert(std::unique(power_set.begin(), power_set.end()) == power_set.end());
		power_set_map.enable_hash_cache();
		power_set_map.reserve(nfa->total_states() * 2);
		power_set_map.risk_size_inc(2);
		power_set_map.risk_size_dec(1);
		power_set_map.fast_begin()[0] = 0; // now size==1
		power_set_map.fast_begin()[1] = power_set.size();
		power_set_map.risk_insert_on_slot(0); // dfa initial_state
		//
		// power_set_map.end_i() may increase in iterations
		// when end_i() didn't changed, ds will catch up, then the loop ends.
		for (size_t ds = 0; ds < power_set_map.end_i(); ++ds) {
			PowerIndex subset_beg = power_set_map.fast_begin()[ds+0];
			PowerIndex subset_end = power_set_map.fast_begin()[ds+1];
			dfa_next.resize0();
			for (PowerIndex j = subset_beg; j < subset_end; ++j) {
				state_id_t nfa_state_id = power_set[j];
				nfa->get_non_epsilon_targets(nfa_state_id, &children);
				for (auto t : children) {
					walker.gen_epsilon_closure(t.target, &dfa_next.bykey(t.ch));
				}
				if (nfa->is_final(nfa_state_id))
				   	dfa.set_term_bit(ds);
			}
			children.erase_all();
			for (size_t k = 0; k < dfa_next.size(); ++k) {
				NFA_SubSet& nss = dfa_next.byidx(k);
				assert(!nss.empty());
				std::sort(nss.begin(), nss.end()); // sorted subset as Hash Key
				nss.trim(std::unique(nss.begin(), nss.end()));
				if (power_set.size() + nss.size() > max_power_set) {
					return 0; // failed
				}
				power_set.append(nss);        // add the new subset as Hash Key
				power_set_map.risk_size_inc(2);
				power_set_map.risk_size_dec(1);
				power_set_map.fast_end()[0] = power_set.size(); // new subset_end
				size_t New = power_set_map.end_i()-1; assert(New > 0);
				size_t Old = power_set_map.risk_insert_on_slot(New);
				if (New != Old) { // failed: the dfa_state: subset has existed
					power_set_map.risk_size_dec(1);
					power_set.resize(power_set_map.fast_end()[0]);
				}
				else if (dfa.total_states() < power_set_map.end_i()) {
					dfa.resize_states(power_set_map.capacity());
				}
			//	printf("New=%zd Old=%zd\n", New, Old);
				children.emplace_back(nss.ch, Old);
			}
			std::sort(children.begin(), children.end());
			dfa.add_all_move(ds, children);
		}
	#ifdef FEBIRD_NFA_TO_DFA_PRINT_HASH_COLLISION
		printf("FEBIRD_NFA_TO_DFA_PRINT_HASH_COLLISION\n");
		valvec<int> collision;
		power_set_map.bucket_histogram(collision);
		for (int i = 0; i < (int)collision.size(); ++i) {
			printf("make_dfa: collision_len=%d cnt=%d\n", i, collision[i]);
		}
	#endif
		dfa.resize_states(power_set_map.end_i());
		power_set.shrink_to_fit();
		if (dfa_states)
			dfa_states->assign(power_set_map.fast_begin(), power_set_map.fast_end()+1);
		return power_set.size();
	}

	template<class Dfa>
	size_t make_dfa(Dfa& dfa, size_t max_power_set = ULONG_MAX) {
		valvec<uint32_t>* dfa_states = NULL;
		valvec<state_id_t> power_set;
		return make_dfa(dfa_states, power_set, dfa, max_power_set);
	}

	void add_path(state_id_t source, state_id_t target, auchar_t ch) {
		nfa->add_move(source, target, ch);
	}
	template<class Char>
	void add_path(state_id_t source, state_id_t target, const basic_fstring<Char> str) {
		assert(source < nfa->total_states());
		assert(target < nfa->total_states());
		if (0 == str.n) {
			nfa->add_epsilon(source, target);
		} else {
			state_id_t curr = source;
			for (size_t i = 0; i < str.size()-1; ++i) {
				auchar_t ch = str.uch(i);
				assert(ch < nfa->get_sigma());
				state_id_t next = nfa->new_state();
				nfa->add_move(curr, next, ch);
				curr = next;
			}
			nfa->add_move(curr, target, str.end()[-1]);
		}
	}

	void add_word(const fstring word) {
		add_word_aux(initial_state, word);
	}
	void add_word(state_id_t RootState, const fstring word) {
		add_word_aux(RootState, word);
	}
	void add_word16(const fstring16 word) {
		add_word_aux(initial_state, word);
	}
	void add_word16(state_id_t RootState, const fstring16 word) {
		add_word_aux(RootState, word);
	}
	template<class Char>
	void add_word_aux(state_id_t RootState, const basic_fstring<Char> word) {
		state_id_t curr = RootState;
		for (size_t i = 0; i < word.size(); ++i) {
			state_id_t next = nfa->new_state();
			auchar_t ch = word.uch(i);
			assert(ch < nfa->get_sigma());
			nfa->add_move(curr, next, ch);
			curr = next;
		}
		nfa->set_final(curr);
	}

	void write_one_state(long ls, FILE* fp) const {
		if (nfa->is_final(ls))
			fprintf(fp, "\tstate%ld[label=\"%ld\" shape=\"doublecircle\"];\n", ls, ls);
		else
			fprintf(fp, "\tstate%ld[label=\"%ld\"];\n", ls, ls);
	}

	void write_dot_file(FILE* fp) const {
		SinglePassPFS_Walker walker(nfa);
		walker.put_root(initial_state);
		fprintf(fp, "digraph G {\n");
		while (!walker.is_finished()) {
			state_id_t curr = walker.next();
			write_one_state(curr, fp);
			walker.discover_epsilon_children(curr);
			walker.discover_non_epsilon_children(curr);
		}
		walker.clear_color();
		walker.put_root(initial_state);
		valvec<CharTarget<size_t> > non_epsilon;
		valvec<state_id_t>     epsilon;
		while (!walker.is_finished()) {
			state_id_t curr = walker.next();
			assert(curr < nfa->total_states());
			nfa->get_epsilon_targets(curr, &epsilon);
			for (size_t j = 0; j < epsilon.size(); ++j) {
				long lcurr = curr;
				long ltarg = epsilon[j];
			//	const char* utf8_epsilon = "\\xCE\\xB5"; // dot may fail to show ε
				const char* utf8_epsilon = "&#949;";
				fprintf(fp, "\tstate%ld -> state%ld [label=\"%s\"];\n", lcurr, ltarg, utf8_epsilon);
			}
			nfa->get_non_epsilon_targets(curr, &non_epsilon);
			for (size_t j = 0; j < non_epsilon.size(); ++j) {
				long lcurr = curr;
				long ltarg = non_epsilon[j].target;
				int  label = non_epsilon[j].ch;
				if ('\\' == label || '"' == label)
					fprintf(fp, "\tstate%ld -> state%ld [label=\"\\%c\"];\n", lcurr, ltarg, label);
				else if (isgraph(label))
					fprintf(fp, "\tstate%ld -> state%ld [label=\"%c\"];\n", lcurr, ltarg, label);
				else
					fprintf(fp, "\tstate%ld -> state%ld [label=\"\\\\x%02X\"];\n", lcurr, ltarg, label);
			}
			walker.discover_epsilon_children(curr);
			walker.discover_non_epsilon_children(curr);
		}
		fprintf(fp, "}\n");
	}

	void write_dot_file(const char* fname) const {
		FILE* fp = fopen(fname, "w");
		if (NULL == fp) {
			fprintf(stderr, "fopen(\"%s\", \"w\") = %s\n", fname, strerror(errno));
			throw std::runtime_error("fopen");
		}
		write_dot_file(fp);
		fclose(fp);
	}
};

template<class NFA, class PowerIndex, class Dfa>
size_t NFA_to_DFA_make_dfa( const NFA& nfa
						, valvec<PowerIndex>* dfa_states
						, valvec<typename NFA::state_id_t>& power_set
						, Dfa& dfa
						, size_t max_power_set = ULONG_MAX)
{
	return NFA_to_DFA<NFA>::s_make_dfa(&nfa, dfa_states, power_set, dfa, max_power_set);
}
template<class NFA, class Dfa>
size_t NFA_to_DFA_make_dfa( const NFA& nfa
						, Dfa& dfa
						, size_t max_power_set = ULONG_MAX)
{
	valvec<typename Dfa::state_id_t>* dfa_states = NULL; // power_index
	valvec<typename NFA::state_id_t> power_set;
	return NFA_to_DFA<NFA>::s_make_dfa(&nfa, dfa_states, power_set, dfa, max_power_set);
}


#endif // __febird_nfa_hpp__

