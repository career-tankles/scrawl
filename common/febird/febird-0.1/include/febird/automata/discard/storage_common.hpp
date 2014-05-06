#ifndef __febird_automata_storage_common_hpp__
#define __febird_automata_storage_common_hpp__

#if defined(__GNUC__)
	#if __GNUC__ > 4 || __GNUC__ == 4 && __GNUC_MINOR__ >= 7
	#else
		#error "Requires GCC-4.7+"
	#endif
#endif

#include <febird/valvec.hpp>
#include <febird/mempool.hpp>
#include <assert.h>
#include <stdio.h>
#include <boost/current_function.hpp>

namespace febird { namespace automata { namespace storage {

template<class T, class Key, class Comp>
size_t idx_lower_bound(const T* p, size_t lo, size_t hi, const Key& key, Comp comp) {
	assert(lo <= hi);
	while (lo < hi) {
		size_t mid = (lo + hi) / 2;
		if (comp(p[mid], key))
			lo = mid + 1;
		else
			hi = mid;
	}
	return lo;
}
template<class T, class Key, class Comp>
size_t idx_upper_bound(const T* p, size_t lo, size_t hi, const Key& key, Comp comp) {
	assert(lo <= hi);
	while (lo < hi) {
		size_t mid = (lo + hi) / 2;
		if (!comp(key, p[mid]))
			lo = mid + 1;
		else
			hi = mid;
	}
	return lo;
}
template<class T, class Key>
size_t idx_lower_bound(const T* p, size_t lo, size_t hi, const Key& key) {
	assert(lo <= hi);
	while (lo < hi) {
		size_t mid = (lo + hi) / 2;
		if (p[mid] < key)
			lo = mid + 1;
		else
			hi = mid;
	}
	return lo;
}
template<class T, class Key>
size_t idx_upper_bound(const T* p, size_t lo, size_t hi, const Key& key) {
	assert(lo <= hi);
	while (lo < hi) {
		size_t mid = (lo + hi) / 2;
		if (!(key < p[mid]))
			lo = mid + 1;
		else
			hi = mid;
	}
	return lo;
}

template<class State>
class top_base {
public:
    typedef typename State::state_id_t state_id_t;
protected:
    MemPool       pool;
    valvec<State> states;
    state_id_t    firstFreeState;
    state_id_t    numFreeStates;
    size_t        transition_num;
	bool          zpath_states;

#define AUTOMATA_USING_TOP_BASE_MEMBERS(top) \
	typedef typename top::state_id_t state_id_t; \
    using top::initial_state; \
	using top::pool; \
	using top::states; \
	using top::firstFreeState; \
	using top::numFreeState; \
	using top::transition_num; \
	using top::zpath_states;

	void reset() {
        states.push_back(); // initial_state
        firstFreeState = null_state;
        numFreeStates  = 0;
        transition_num = 0;
		zpath_states = false;
	}
	explicit top_base() : pool(512) { reset(); }

public:
	const valvec<State>& internal_get_states() const { return states; }
	const MemPool      & internal_get_pool  () const { return pool  ; }

    size_t total_states() const { return states.size(); }
    size_t total_transitions() const { return transition_num; }

	size_t num_free_states() const { return numFreeStates; }
	size_t num_used_states() const { return states.size() - numFreeStates; }

	void clear() {
	   	states.clear();
	   	pool.clear();
	   	reset();
	}

    state_id_t new_state() {
        state_id_t s;
        if (null_state == firstFreeState) {
            s = states.size();
            if (s >= State::max_state) {
                throw std::runtime_error("Automata::new_state(), exceed State::max_state");
            }
            states.push_back(State());
        } else {
            assert(numFreeStates > 0);
            numFreeStates--;
            s = firstFreeState;
            firstFreeState = states[s].get_target();
            new(&states[s])State(); // call default constructor
        }
        return s;
    }

    bool is_final(state_id_t s) const {
        assert(s < (state_id_t)states.size());
        return states[s].is_final();
    }
    void set_final_bit(state_id_t s) {
        assert(s < (state_id_t)states.size());
        states[s].set_final_bit();
    }
};

} } } // namespace febird::automata

#endif // __febird_automata_storage_common_hpp__

