#ifndef __febird_automata_storage_bin_search_hpp__
#define __febird_automata_storage_bin_search_hpp__

#if defined(__GNUC__)
	#if __GNUC__ > 4 || __GNUC__ == 4 && __GNUC_MINOR__ >= 7
	#else
		#error "Requires GCC-4.7+"
	#endif
#endif

#include <febird/valvec.hpp>
#include <febird/mempool.hpp>
#include <febird/bitmap.hpp>
#include <febird/smallmap.hpp>

#include <assert.h>
#include <stdio.h>

#include <algorithm>
#include <functional>
#include <vector>
#include <string>
#include <queue>
#include <stack>
#include <map>
#include <set>
#include <typeinfo>

#include <febird/hash_strmap.hpp>
#include <boost/current_function.hpp>
#include <boost/type_traits/is_unsigned.hpp>

namespace febird { namespace automata { namespace storage {

template<class StateID, unsigned Sigma = 256>
class BS_State {
public:
	StateID  trans_000 : sizeof(StateID)*8 - 1;
	unsigned final_bit : 1;
	StateID  xpos      : sizeof(StateID)*8 - 1;
	unsinged pzip_bit  : 1;

    typedef StateID  transition_t; // could be overrided by derived class
    typedef StateID  state_id_t;
    typedef StateID  position_t;
    static const state_id_t nil_state = ~(state_id_t)0 >> 1;
    static const state_id_t  max_state = nil_state;

    BS_State() {
        trans_000 = nil_state;
		xpos = max_state;
		final_bit = 0;
		pzip_bit = 0;
    }

	bool is_single() const { return xpos < Sigma; }
    bool is_final() const { return final_bit; }
	bool is_pzip() const { return pzip_bit; }
    void set_final_bit() { final_bit = 1; }
	void set_pzip_bit() { pzip_bit = 1; }
    void setpos(size_t tpos) {
        if (tpos >= size_t(nil_state - Sigma) * MemPool::align_size) {
			throw std::logic_error("BS_State::setpos: too large tpos");
		}
        xpos = Sigma + position_t(tpos / MemPool::align_size);
    }
    size_t getpos() const {
		assert(xpos >= Sigma);
        return size_t(xpos - Sigma) * MemPool::align_size;
    }

	static state_id_t target_of(StateID x) { return x; }
	StateID first_trans() const { return trans_000; }
};

template<class BaseState>
class DAWG_State : public BaseState {
public:
	typedef typename BaseState::state_id_t state_id_t;
	struct transtion_t {
		state_id_t t;
		state_id_t num;
	};
	static state_id_t target_of(const transtion_t& x) { return x.t; }

	transition_t first_trans() const {
		transtion_t x;
		x.t = this->trans_000;
		x.num = 0;
		return x;
	}
};

/// State must has trivial destructor
template< class Char
		, class State
		, unsigned Sigma = 1u << sizeof(Char)*8
		>
class use_array : public top_base<State> {
	typedef top_base<State> super;
	BOOST_STATIC_ASSERT(boost::is_unsigned<Char>::value);

public:
	typedef State    state_t;
	typedef typename State::transition_t transition_t;
    typedef typename State::position_t   position_t;
    typedef typename State::state_id_t   state_id_t;
	static const  unsigned sigma = Sigma;
    static const  unsigned tsize = sizeof(Tran);
    static const  state_id_t  max_state = State::max_state;
    static const  state_id_t nil_state = State::null_state;

	BOOST_STATIC_ASSERT(boost::is_unsigned<state_id_t>::value);

protected:
	struct StateData {
		Char  n_trans_1; // value of (actual_transition_num - 1)
		Char  maxch;
		Char  ch_arr[1]; // array length is n_trans_1, don't include maxch
						 // after ch_arr, there is transition array

		      Tran* trans()       { return (      Trans*)(ch_arr + n_trans_1 + 1); }
		const Tran* trans() const { return (const Trans*)(ch_arr + n_trans_1 + 1); }

		Tran& trans(size_t i)       {
			assert(i <= (size_t)n_trans_1);
			assert(i >= 1); // first transtion is saved in State
			return ((Trans*)(ch_arr + n_trans_1))[i-1];
		}
		const Tran& trans(size_t i) const {
			assert(i <= (size_t)n_trans_1);
			assert(i >= 1); // first transtion is saved in State
		   	return ((const Trans*)(ch_arr + n_trans_1))[i-1];
		}

		size_t n_trans() const { return size_t(n_trans_1) + 1; }

		void n_trans_set(size_t n) {
		   	assert(n >= 2);
		   	assert(n <= Sigma);
		   	n_trans_1 = Char(n - 1);
		}

		size_t mem_size() const {
			return offsetof(StateData, ch_arr) + n_trans_1 * (sizeof(Char) + sizeof(Tran));
		}
		static size_t mem_size(size_t n_trans) {
			return offsetof(StateData, ch_arr) + (n_trans-1) * (sizeof(Char) + sizeof(Tran));
		}
	};

public:
    state_id_t clone_state(state_id_t source) {
        assert(source < this->states.size());
        state_id_t  y = this->new_state();
        const State s = this->states[source];
		this->states[y] = s;
        if (!s.is_single())  {
			size_t pos1 = s.getpos();
			const auto* src = this->pool.template at<StateData>(pos1);
			size_t bytes = src->mem_size();
			size_t pos2 = this->pool.alloc(bytes);
			memcpy(this->pool.ptr(pos2), this->pool.ptr(pos1), bytes);
			this->states[y].setpos(pos2);
			transition_num += src->n_trans();
        }
		else if (s.is_pzip()) {
			// very unlikely:
			assert(0); // now, unreachable
		}
		else if (nil_state != s.get_target()) {
			transition_num += 1;
		}
		return y;
    }

    void del_state(state_id_t s) {
		assert(!this->zpath_states);
        assert(s < this->states.size());
        assert(s != initial_state); // initial_state shouldn't be deleted
		State& x = states[s];
		assert(!x.is_pzip()); // must be non-path-zipped
		if (!x.is_single()) { // has transitions in pool
			size_t pos1 = s.getpos();
			const auto* src = this->pool.template at<StateData>(pos1);
            this->pool.sfree(pos, src->mem_size());
        }
	   	else if (x.get_target() != nil_state)
			transition_num -= 1; // just 1 transition
		// put s to free list, states[s] is not a 'State' from now
		x.set_target(firstFreeState);
		firstFreeState = s;
		numFreeStates++;
    }

    Tran state_move(const State& s, Char ch) const {
        if (s.is_single())
			return s.first_trans();
		size_t pos1 = s.getpos();
		const auto* sd = &this->pool.template at<StateData>(pos1);
		size_t n_trans = sd->n_trans();
		size_t idx = idx_lower_bound(sd->ch_arr, 0, n_trans-1, ch);
		if (idx < n_trans-1) {
			if (ch == arr[1+idx]) {
				if (0 == idx)
					return s.first_trans(s);
				else
					return sd->trans(idx);
			}
		}
		else if (sd->maxch == ch) {
			return sd->trans(idx);
		}
        return nil_state;
    }

protected:
    state_id_t add_move_imp(const state_id_t source, const state_id_t target
			, const Char ch, bool OverwriteExisted) {
        assert(source < (state_id_t)this->states.size());
        assert(target < (state_id_t)this->states.size());
		assert(!this->states[source].is_pzip());
	//	assert(!this->states[target].is_pzip()); // target could be path zipped
        State& s = this->states[source];
        if (s.xpos < sigma) {
			const state_id_t old_target = s.trans_000;
            if (nil_state == old_target) {
                s.xpos = ch;
                s.trans_000 = target;
				transition_num++;
				return nil_state;
            }
            if (s.xpos == ch) {
                // the target for ch has existed
				if (OverwriteExisted)
					s.trans_000 = target;
                return old_target;
            }
			Char minch = std::min<Char>(s.xpos, ch);
			Char maxch = std::max<Char>(s.xpos, ch); 
			assert(0 != minch); // temporary assert
			size_t n_trans = 2;
            size_t pos =  this->pool.alloc(StateData::mem_size(n_trans));
			auto*  pt  = &this->pool.template at<StateData>(pos);
            if (ch == minch) {
				s.trans_000 = target;
				new(&pt->trans(1))Tran(old_target);
            } else {
				new(&pt->trans(1))Tran(target);
			}
			pt->n_trans_set(n_trans);
			pt->maxch = maxch;
			pt->ch_arr[0] = minch;
            s.setpos(pos);
            check_slots(s, n_trans);
			transition_num++;
            return nil_state;
        }
		size_t old_pos = s.getpos();
		auto*  old    = &this->pool.template at<StateData>(old_pos);
		size_t n_tran = old->n_trans();
		size_t cbytes = this->pool.align_to(sizeof(Char)*(n_tran+1));
		size_t tbytes = this->pool.align_to(sizeof(Tran)*(n_tran-1));
		size_t idx = idx_lower_bound(old->ch_arr, 0, n_tran-1, ch);
		Tran*  pt  = &this->pool.template at<Tran>(cbytes);
		if (idx < n_trans-1 && ch == old->ch_arr[idx] || old->maxch == ch) {
			// the transition has existed
			state_id_t old_target;
			if (OverwriteExisted) {
				if (0 == idx)
					old_target = s.trans_000, s.trans_000 = target;
				else
					old_target = pt[idx-1], new(pt+idx-1)Tran(target);
			} else {
				if (0 == idx)
					old_target = s.trans_000;
				else
					old_target = pt[idx-1];
			}
			return old_target;
		}
		size_t cbytes2 = this->pool.align_to(sizeof(Char)*(n_tran+1+1));
		size_t tbytes2 = this->pool.align_to(sizeof(Tran)*(n_tran-1+1));
		size_t new_pos = this->pool.realloc(cbytes2 + tbytes2);
		pc = &this->pool.template at<Char>(new_pos);
		Trans* pt1 = &this->pool.template at<Tran>(new_pos + cbytes);
		Trans* pt2 = &this->pool.template at<Tran>(new_pos + cbytes2);
		if (0 == idx) {
			memmove(pt2+1, pt1, sizeof(Char)*(n_tran-1));
			new(pt2)Tran(s.trans_000);
			s.trans_000 = target;
		} else {
			memmove(pt2+idx+1, pt1+idx, sizeof(Char)*(n_tran-idx-1));
			new(pt2+idx)Tran(target); // insert target
		}
		memmove(pc+1+idx+1, pc+1+idx, sizeof(Char)*(n_tran-idx));
		pc[1+idx] = ch; // insert ch
        assert(nil_state != s.trans_000);
        transition_num++;
		return nil_state;
    }
public:
	size_t trans_num(state_id_t curr) const {
        const State s = states[curr];
        if (s.xpos < sigma) {
            state_id_t target = s.trans_000;
			return (nil_state == target) ? 0 : 1;
        }
		return this->pool.template at<Char>(s.getpos()) + size_t(1);
	}

	size_t trans_bytes(const State& s) const {
        if (s.is_single()) {
			assert(s.is_pzip());
			return sizeof(state_id_t);
		}
		size_t pos = s.getpos();
		Char*  pc     = &this->pool.template at<Char>(pos);
		size_t n_tran = pc[0] + size_t(1);
		size_t cbytes = this->pool.align_to(sizeof(Char)*(n_tran+1));
		size_t tbytes = this->pool.align_to(sizeof(Tran)*(n_tran-1));
		return cbytes + tbytes;
	}

	state_id_t single_target(state_id_t s) const { return single_target(states[s]); }
	state_id_t single_target(const State& s) const {
		// just 1 trans, if not ziped, needn't extra memory in pool
		// default trans_000 is nil_state, just the designated
		assert(s.xpos < sigma); // must be single target
		return s.is_pzip() ? pool.at<state_id_t>(s.trans_000) : s.trans_000;
	}

    // just walk the (curr, dest), don't access the 'ch'
    // this is useful such as when minimizing DFA
    template<class OP> // use ctz (count trailing zero)
    void for_each_dest(state_id_t curr, OP op) const {
        const State s = states[curr];
        if (s.xpos < sigma) {
            state_id_t target = single_target(s);
            if (nil_state != target)
                op(curr, first_trans(s, target, (Tran*)NULL));
            return;
        }
		size_t pos = s.getpos();
		Char*  pc = &this->pool.template at<Char>(pos);
		size_t n_tran = pc[0] + size_t(1);
		size_t cbytes = this->pool.align_to(sizeof(Char)*(n_tran+1));
		Trans* pt = &this->pool.template at<Tran>(pos + cbytes);
        for (size_t i = 0; i < n_tran; ++i)
            op(curr, pt[i]);
    }

    template<class OP> // use ctz (count trailing zero)
    void for_each_move(state_id_t curr, OP op) const {
        const State s = states[curr];
        if (s.xpos < sigma) {
            state_id_t target = single_target(s);
            if (nil_state != target)
                op(curr, s.first_trans(s), s.xpos);
            return;
        }
		size_t pos = s.getpos();
		Char*  pc = &this->pool.template at<Char>(pos);
		size_t n_tran = pc[0] + size_t(1);
		size_t cbytes = this->pool.align_to(sizeof(Char)*(n_tran+1));
		Trans* pt = &this->pool.template at<Tran>(pos + cbytes);
		op(curr, Tran(s, s.trans_000), pc[0]);
        for (size_t i = 0; i < n_tran; ++i)
            op(curr, pt[i], pc[1+i]);
    }

	// exit immediately if op returns false
    template<class OP> // use ctz (count trailing zero)
    void for_each_move_break(state_id_t curr, OP op) const {
        const State s = states[curr];
        if (s.xpos < sigma) {
            state_id_t target = single_target(s);
            if (nil_state != target) {
				Char ch = (Char)s.xpos;
                op(curr, s.first_trans(), ch);
			}
            return;
        }
		size_t pos = s.getpos();
		Char*  pc = &this->pool.template at<Char>(pos);
		size_t n_tran = pc[0] + size_t(1);
		size_t cbytes = this->pool.align_to(sizeof(Char)*(n_tran+1));
		Trans* pt = &this->pool.template at<Tran>(pos + cbytes);
		if (!op(curr, Tran(s, s.trans_000), pc[0]))
			return;
        for (size_t i = 0; i < n_tran; ++i)
            if (!op(curr, pt[i], pc[1+i]))
				return;
    }
};

} } } // namespace febird::automata

#endif // __febird_automata_storage_bin_search_hpp__

