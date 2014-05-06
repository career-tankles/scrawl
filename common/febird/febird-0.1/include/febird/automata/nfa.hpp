#ifndef __febird_nfa_hpp__
#define __febird_nfa_hpp__

#include <febird/fstring.hpp>
#include <febird/gold_hash_map.hpp>
#include <febird/valvec.hpp>
#include <febird/bitmap.hpp>
#include <febird/smallmap.hpp>
#include <stdio.h>
#include "dfa_interface.hpp"

template<class StateID>
struct CharTarget {
	StateID  ch;
	StateID  target;
	static const StateID bad_char = StateID(~(StateID)0);
	CharTarget(StateID c, StateID t) : ch(c), target(t) {}
	friend bool operator<(CharTarget x, CharTarget y) {
		if (x.ch != y.ch)
			return x.ch < y.ch;
		else
			return x.target < y.target;
	}
	friend bool operator==(CharTarget x, CharTarget y) {
		return x.ch == y.ch && x.target == y.target;
	}
	friend bool operator!=(CharTarget x, CharTarget y) {
		return x.ch != y.ch || x.target != y.target;
	}
};

template<class DFA, class CharTargetVec>
void DFA_get_all_transitions(const DFA& dfa, typename DFA::state_id_t s, CharTargetVec* res) {
	typedef typename DFA::state_id_t state_id_t;
	res->erase_all();
	dfa.for_each_move(s, [res](state_id_t, state_id_t t, unsigned char c) {
		res->push_back(CharTarget<state_id_t>(c, t));
	});
}

struct NFA_Transition4B {
	NFA_Transition4B(uint32_t target, unsigned char ch) {
		this->target = target;
		this->ch     = ch;
	}
	uint32_t target : 24;
	unsigned ch     :  8;
	typedef uint32_t state_id_t;
	typedef signed char  char_t;
	static const size_t  sigma = 256;
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
	typedef typename Transition::char_t char_t;
	typedef std::pair<const Transition*, const Transition*> target_range_t;
	typedef std::pair<      Transition*,       Transition*> target_range_tm; // mutable
	const static size_t sigma = Transition::sigma;

	GenericNFA() {
		states.resize(1); // make initial_state
	}

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
	void add_move(state_id_t source, state_id_t target, char_t ch) {
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

	void get_all_targets(state_id_t s, valvec<CharTarget<state_id_t> >* children) const {
		assert(s < states.size());
		const NfaState& ts = states[s];
		children->erase_all();
		children->reserve(ts.targets.size());
		const state_id_t bad_char = CharTarget<state_id_t>::bad_char;
		for (size_t i = 0; i < ts.n_epsilon; ++i)
			children->unchecked_push_back(
				CharTarget<state_id_t>(bad_char, ts.targets[i].target));
		for (size_t i = ts.n_epsilon; i < ts.targets.size(); ++i)
			children->unchecked_push_back(
				CharTarget<state_id_t>(ts.targets[i].ch, ts.targets[i].target));
	}
	void get_epsilon_targets(state_id_t s, valvec<state_id_t>* children) const {
		assert(s < states.size());
		const NfaState& ts = states[s];
		children->erase_all();
		children->reserve(ts.n_epsilon);
		for (size_t i = 0; i < ts.n_epsilon; ++i)
			children->unchecked_push_back(ts.targets[i].target);
	}
	void get_non_epsilon_targets(state_id_t s, valvec<CharTarget<state_id_t> >* children) const {
		assert(s < states.size());
		const NfaState& ts = states[s];
		children->erase_all();
		children->reserve(ts.targets.size() - ts.n_epsilon);
		for (size_t i = ts.n_epsilon; i < ts.targets.size(); ++i)
			children->unchecked_push_back(
				CharTarget<state_id_t>(ts.targets[i].ch, ts.targets[i].target));
	}
};

// a NFA which is derived from a DFA, there are few states of the NFA
// are Non-Determinal, most states are determinal
// Non-Determinal states are save in a hash map
template<class DFA>
class NFA_BaseOnDFA {
public:
	typedef typename DFA::state_id_t state_id_t;
	static const int sigma = DFA::sigma;
private:
	struct NfaState {
		valvec<CharTarget<state_id_t> > targets;
		valvec<state_id_t>  epsilon;
	};
	DFA* dfa;
	gold_hash_map<state_id_t, NfaState> nfa;

public:
	NFA_BaseOnDFA(DFA* dfa1) : dfa(dfa1) {}

	size_t total_states() const { return dfa->total_states(); }

	state_id_t new_state() { return dfa->new_state(); }

	/// will not add duplicated (targets, ch)
	/// @param ch the transition label char
	void add_move(state_id_t source, state_id_t target, state_id_t ch) {
		assert(ch < sigma);
		state_id_t existed_target = dfa->add_move(source, target, ch);
		if (DFA::null_state == existed_target || target == existed_target)
			return; // Good luck, it is determinal
		auto& targets = nfa[source].targets;
		CharTarget<state_id_t> char_target(ch, target);
		auto  ins_pos = std::lower_bound(targets.begin(), targets.end(), char_target);
		if (targets.end() == ins_pos || char_target < *ins_pos)
			targets.insert(ins_pos, char_target);
	}

	/// will not add duplicated targets
	void add_epsilon(state_id_t source, state_id_t target) {
		auto& epsilon = nfa[source].epsilon;
		auto  ins_pos = std::lower_bound(epsilon.begin(), epsilon.end(), target);
		if (epsilon.end() == ins_pos || target < *ins_pos) {
		//	fprintf(stderr, "add_epsilon: source=%u target=%u\n", source, target);
			epsilon.insert(ins_pos, target);
		}
	}

	/// may add duplicated targets
	void push_epsilon(state_id_t source, state_id_t target) {
		nfa[source].epsilon.push_back(target);
	}
	void sort_epsilon(state_id_t source, state_id_t target) {
		nfa[source].epsilon.push_back(target);
		auto& eps = nfa[source].epsilon;
		std::sort(eps.begin(), eps.end());
	}

	bool is_final(state_id_t s) const {
		assert(s < dfa->total_states());
		return dfa->is_term(s);
	}
	void set_final(state_id_t s) {
		assert(s < dfa->total_states());
		dfa->set_term(s);
	}

	void get_dfa_targets(state_id_t s, valvec<CharTarget<state_id_t> >* children) const {
		typedef typename DFA::state_id_t DState;
		dfa->for_each_move(s, [&](state_id_t, state_id_t target, state_id_t ch) {
			children->push_back(CharTarget<state_id_t>(ch, target));
		});
	}

	void get_all_targets(state_id_t s, valvec<CharTarget<state_id_t> >* children) const {
		assert(s < dfa->total_states());
		children->erase_all();
		size_t idx = nfa.find_i(s);
		if (nfa.end_i() != idx) {
			auto& targets = nfa.val(idx).targets;
			auto& epsilon = nfa.val(idx).epsilon;
			children->reserve(targets.size() + epsilon.size());
			const state_id_t bad_char = CharTarget<state_id_t>::bad_char;
			for (auto target : targets)
				children->unchecked_push_back(CharTarget<state_id_t>(bad_char, target));
			children->append(targets);
		}
		get_dfa_targets(s, children);
	}
	void get_epsilon_targets(state_id_t s, valvec<state_id_t>* children) const {
		assert(s < dfa->total_states());
		children->erase_all();
		size_t idx = nfa.find_i(s);
		if (nfa.end_i() != idx) {
			auto& epsilon = nfa.val(idx).epsilon;
			children->assign(epsilon.begin(), epsilon.end());
		//	fprintf(stderr, "get_epsilon_targets(%u):", s);
		//	for (state_id_t t : epsilon) fprintf(stderr, " %u", t);
		//	fprintf(stderr, "\n");
		}
	}
	void get_non_epsilon_targets(state_id_t s, valvec<CharTarget<state_id_t> >* children) const {
		assert(s < dfa->total_states());
		children->erase_all();
		size_t idx = nfa.find_i(s);
		if (nfa.end_i() != idx) {
			auto& targets = nfa.val(idx).targets;
			const state_id_t bad_char = CharTarget<state_id_t>::bad_char;
			children->assign(targets.begin(), targets.end());
		}
		get_dfa_targets(s, children);
	}

	void concat(state_id_t root1, state_id_t root2) {
		typename DFA::pfs_walker walker;
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
};

struct powerkey_t {
	size_t offset;
	size_t length;

	powerkey_t(size_t offset1, size_t length1)
	   	: offset(offset1), length(length1) {}
	powerkey_t() : offset(), length() {}
};


template<class NFA>
class NFA_to_DFA {
	NFA* nfa;

public:
	NFA_to_DFA(NFA* nfa1) : nfa(nfa1) {}

	typedef typename NFA::state_id_t  state_id_t;

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

	struct PowerSetHashEq {
		bool equal(powerkey_t x, powerkey_t y) const {
			assert(x.length > 0);
			assert(x.offset + x.length <= power_set.size());
			assert(y.length > 0);
			assert(y.offset + y.length <= power_set.size());
			if (x.length != y.length) {
				return false;
			}
			const state_id_t* p = &power_set[0];
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
			const state_id_t* p = &power_set[x.offset];
			size_t h = 0;
			size_t n = x.length;
			do {
				h += *p++;
				h *= 31;
				h += h << 3 | h >> (sizeof(h)*8-3);
			} while (--n);
			return h;
		}
		explicit PowerSetHashEq(const valvec<state_id_t>& power_set)
		 : power_set(power_set) {}

		const valvec<state_id_t>& power_set;
	};

	class DFS_Walker {
		const NFA*          nfa;
		valvec<size_t>      mark;
		valvec<state_id_t>  stack;
		size_t				mark_id;
		valvec<CharTarget<state_id_t> >  cache;
		valvec<state_id_t>  cache_epsilon;

		void cache_to_stack() {
			while (!cache.empty()) {
				state_id_t  target = cache.back().target;
				assert(mark[target] <= mark_id);
				if (mark[target] < mark_id) {
					mark[target] = mark_id;
					stack.push_back(target);
				}
				cache.pop_back();
			}
		}

	public:
		DFS_Walker(const NFA* nfa1)
		  : nfa(nfa1)
		  , mark(nfa1->total_states())
		  , mark_id(0)
	       	{}
		void start(state_id_t root) {
			++mark_id;
			mark[root] = mark_id;
			stack.push_back(root);
		}
		void discover_all_children(state_id_t curr) {
			nfa->get_all_targets(curr, &cache);
			cache_to_stack();
		}
		void discover_epsilon_children(state_id_t curr) {
			nfa->get_epsilon_targets(curr, &cache_epsilon);
			while (!cache_epsilon.empty()) {
				state_id_t  target = cache_epsilon.back();
				assert(mark[target] <= mark_id);
				if (mark[target] < mark_id) {
					mark[target] = mark_id;
					stack.push_back(target);
				}
				cache_epsilon.pop_back();
			}
		}
		void discover_non_epsilon_children(state_id_t curr) {
			nfa->get_non_epsilon_targets(curr, &cache);
			cache_to_stack();
		}
		state_id_t next() {
		   	assert(!stack.empty());
			state_id_t s = stack.back();
			stack.unchecked_pop_back();
		   	return s;
	   	}
		bool is_finished() const { return stack.empty(); }

		// DFS search for epsilon closure
		void gen_epsilon_closure(state_id_t source, valvec<state_id_t>* closure) {
			start(source);
			while (!is_finished()) {
				state_id_t curr = next();
				closure->push_back(curr);
				discover_epsilon_children(curr);
			}
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
		dfa.erase_all();
		power_set.erase_all();
		// must be FastCopy/realloc
		typedef node_layout<PowerIndex, PowerIndex, FastCopy, ValueInline> Layout;
		gold_hash_tab<powerkey_t, PowerIndex, PowerSetHashEq, PowerSetKeyExtractor<PowerIndex>, Layout>
			power_set_map(3, PowerSetHashEq(power_set), PowerSetKeyExtractor<PowerIndex>());
		smallmap<NFA_SubSet> dfa_next(256); // collect the targets of a dfa state
		DFS_Walker walker(nfa);
		valvec<CharTarget<state_id_t> > children;
		walker.gen_epsilon_closure(initial_state, &power_set);
		std::sort(power_set.begin(), power_set.end());
		power_set_map.enable_hash_cache();
		power_set_map.reserve(nfa->total_states() * 2);
		power_set_map.risk_size_inc(2);
		power_set_map.risk_size_dec(1);
		power_set_map.begin()[0] = 0; // now size==1
		power_set_map.begin()[1] = power_set.size();
		power_set_map.risk_insert_on_slot(0); // dfa initial_state
		//
		// power_set_map.end_i() may increase in iterations
		// when end_i() didn't changed, ds will catch up, then the loop ends.
		for (size_t ds = 0; ds < power_set_map.end_i(); ++ds) {
			PowerIndex subset_beg = power_set_map.begin()[ds+0];
			PowerIndex subset_end = power_set_map.begin()[ds+1];
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
			for (size_t k = 0; k < dfa_next.size(); ++k) {
				NFA_SubSet& nss = dfa_next.byidx((int)k);
				assert(!nss.empty());
				std::sort(nss.begin(), nss.end()); // sorted subset as Hash Key
				nss.trim(std::unique(nss.begin(), nss.end()));
				if (power_set.size() + nss.size() > max_power_set) {
					return 0; // failed
				}
				power_set.append(nss);        // add the new subset as Hash Key
				power_set_map.risk_size_inc(2);
				power_set_map.risk_size_dec(1);
				power_set_map.end()[0] = power_set.size(); // new subset_end
				size_t New = power_set_map.end_i()-1; assert(New > 0);
				size_t Old = power_set_map.risk_insert_on_slot(New);
				if (New != Old) { // failed: the dfa_state: subset has existed
					power_set_map.risk_size_dec(1);
					power_set.resize(power_set_map.end()[0]);
				}
				else if (dfa.total_states() < power_set_map.end_i()) {
					dfa.resize_states(power_set_map.capacity());
				}
			//	printf("New=%zd Old=%zd\n", New, Old);
				typename Dfa::state_id_t  ret = dfa.add_move(ds, Old, nss.ch);
			   	assert(Dfa::null_state == ret || ret == Old); (void)ret;
			//	printf("dfa_%zu --(%c)-> dfa_%zu : %d\n", ds, nss.ch, Old, ret);
			}
			//printf("---------\n");
		}
		dfa.resize_states(power_set_map.end_i());
		power_set .shrink_to_fit();
		if (dfa_states)
			dfa_states->assign(power_set_map.begin(), power_set_map.end()+1);
		return power_set.size();
	}

	template<class Dfa>
	size_t make_dfa(Dfa& dfa, size_t max_power_set = ULONG_MAX) {
		valvec<uint32_t>* dfa_states = NULL;
		valvec<state_id_t> power_set;
		return make_dfa(dfa_states, power_set, dfa, max_power_set);
	}

	void add_path(state_id_t source, state_id_t target, const char ch) {
		nfa->add_move(source, target, ch);
	}
	void add_path(state_id_t source, state_id_t target, const fstring str) {
		assert(source < nfa->total_states());
		assert(target < nfa->total_states());
		if (0 == str.n) {
			nfa->add_epsilon(source, target);
		} else {
			state_id_t curr = source;
			for (size_t i = 0; i < str.size()-1; ++i) {
				state_id_t next = nfa->new_state();
				nfa->add_move(curr, next, str.p[i]);
				curr = next;
			}
			nfa->add_move(curr, target, str.end()[-1]);
		}
	}

	void add_word(const fstring word) {
		add_word(initial_state, word);
	}
	void add_word(state_id_t RootState, const fstring word) {
		state_id_t curr = RootState;
		for (size_t i = 0; i < word.size(); ++i) {
			state_id_t next = nfa->new_state();
			nfa->add_move(curr, next, word.p[i]);
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
		DFS_Walker walker(nfa);
		walker.start(initial_state);
		fprintf(fp, "digraph G {\n");
		while (!walker.is_finished()) {
			state_id_t curr = walker.next();
			write_one_state(curr, fp);
			walker.discover_all_children(curr);
		}
		walker.start(initial_state);
		valvec<CharTarget<state_id_t> > non_epsilon;
		valvec<state_id_t>     epsilon;
		while (!walker.is_finished()) {
			state_id_t curr = walker.next();
			assert(curr < nfa->total_states());
			nfa->get_epsilon_targets(curr, &epsilon);
			for (size_t j = 0; j < epsilon.size(); ++j) {
				long lcurr = curr;
				long ltarg = epsilon[j];
			//	const char* utf8_epsilon = "\xCE\xB5"; // dot may fail to show ε
				const char* utf8_epsilon = "eps";
				fprintf(fp, "\tstate%ld -> state%ld [label=\"%s\"];\n", lcurr, ltarg, utf8_epsilon);
			}
			nfa->get_non_epsilon_targets(curr, &non_epsilon);
			for (size_t j = 0; j < non_epsilon.size(); ++j) {
				long lcurr = curr;
				long ltarg = non_epsilon[j].target;
				int  label = non_epsilon[j].ch;
				fprintf(fp, "\tstate%ld -> state%ld [label=\"%c\"];\n", lcurr, ltarg, label);
			}
			walker.discover_all_children(curr);
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

