#ifndef __automata_penglei__
#define __automata_penglei__

#include <assert.h>
#include <stdio.h>

#include <algorithm>
#include <functional>
#include <vector>
#include <string>
#include <map>
#include <set>
#include <typeinfo>

#include <febird/num_to_str.hpp>

#include "nfa.hpp"
#include "dfa_algo.hpp"

#ifdef  ENABLE_DUP_PATH_POOL
	#define IF_DUP_PATH_POOL(stmt) stmt
	#include <febird/hash_strmap.hpp>
#else
	#define IF_DUP_PATH_POOL(stmt)
#endif

// When MyBytes is 6, intentional waste a bit: sbit is 32, nbm is 5
#pragma pack(push,1)
template<class StateID, int MyBytes>
class PackedState {
	enum { sbits  = MyBytes == 6 ? 32 : 8 * MyBytes - 14 IF_DUP_PATH_POOL(-1) };
	StateID  spos     : sbits; // spos: saved position
	unsigned nbm      : MyBytes == 6 ? 6 IF_DUP_PATH_POOL(-1) : 4; // number of uint32 for bitmap, in [0, 9)
	unsigned term_bit : 1;
	unsigned pzip_bit : 1; // is path compressed(zipped)?
	IF_DUP_PATH_POOL(unsigned pool_bit : 1;) // is zipped path in pool?
	unsigned minch    : 8;

public:
    typedef StateID  state_id_t;
    typedef StateID  position_t;
    static const state_id_t null_state = MyBytes == 6 ? UINT_MAX : state_id_t(uint64_t(1) << sbits) - 1;
    static const state_id_t  max_state = null_state; // max_state <= null_state, and could be overridden

    PackedState() {
        spos = null_state;
		nbm = 0;
		term_bit = 0;
		pzip_bit = 0;
		IF_DUP_PATH_POOL(pool_bit = 0;)
        minch = 0;
    }

	void setch(char_t ch) {
	   	minch = ch;
		nbm = 0;
   	}
	void setMinch(char_t ch) {
		assert(ch < minch);
		char_t maxch = (0 == nbm) ? minch : getMaxch();
		nbm = (maxch - ch) / 32 + 1;
		minch = ch;
   	}
	void setMaxch(char_t ch) {
		assert(ch > minch);
		assert(ch > getMaxch());
		nbm = (ch - minch) / 32 + 1;
   	}
	char_t getMaxch() const {
	   	if (0 == nbm)
		   return minch;
	   	unsigned maxch = minch + (nbm*32 - 1);
		return std::min(255u, maxch);
   	}
	char_t getMinch() const { return minch; }
	bool  is_single() const { return 0 == nbm; }
	bool  range_covers(char_t ch) const { return ch == minch || (ch > minch && ch < minch + nbm*32); }
	int   rlen() const { assert(!is_single()); return nbm * 32; }
	bool  has_children() const { return 0 != nbm || spos != null_state; }

    bool is_term() const { return term_bit; }
	bool is_pzip() const { return pzip_bit; }
	bool is_free() const { return 0xF == nbm; }
    void set_term_bit() { term_bit = 1; }
	void set_pzip_bit() { pzip_bit = 1; }
    void clear_term_bit() { term_bit = 0; }
	void set_free() { nbm = 0xF; }
#ifdef  ENABLE_DUP_PATH_POOL
	bool is_pool() const { return pool_bit; }
	void set_pool_bit() { pool_bit = 1; }
#endif
    void setpos(size_t tpos) {
        assert(tpos % MemPool::align_size == 0);
		const size_t limit = size_t(null_state) * MemPool::align_size;
        if (tpos >= limit) {
			string_appender<> msg;
			msg << BOOST_CURRENT_FUNCTION << ": too large tpos=" << tpos
				<< ", limit=" << limit;
			throw std::logic_error(msg);
		}
        spos = position_t(tpos / MemPool::align_size);
    }
    size_t getpos() const { return size_t(spos) * MemPool::align_size; }
    state_id_t get_target() const { return spos; }
    void set_target(state_id_t t) { spos = t; }

	typedef state_id_t transition_t;
	static transition_t first_trans(state_id_t t) { return t; }
}; // end class PackedState
#pragma pack(pop)

typedef PackedState<uint32_t, 4> State4B; ///< 4 byte per-state, max_state is 128K-1 or 256K-1
typedef PackedState<uint32_t, 5> State5B; ///< 5 byte per-state, max_state is  32M-1 or  64M-1
typedef PackedState<uint32_t, 6> State6B; ///< 6 byte per-state, max_state is   4G-1
typedef PackedState<uint64_t, 7> State7B; ///< 7 byte per-state, max_state is   2T-1
BOOST_STATIC_ASSERT(sizeof(State4B) == 4);
BOOST_STATIC_ASSERT(sizeof(State5B) == 5);
BOOST_STATIC_ASSERT(sizeof(State6B) == 6);
BOOST_STATIC_ASSERT(sizeof(State7B) == 7);

/// 8 byte per-state, max_state is 4G-1
class State32 {
    // spos: saved position
    uint32_t spos; // spos*pool.align_size is the offset to MemPool
    char_t   minch;  // min char
    char_t   maxch;  // max char, inclusive
    unsigned term_bit : 1;
	unsigned pzip_bit : 1;
	unsigned free_bit : 1;
	IF_DUP_PATH_POOL(unsigned pool_bit : 1;)

public:
    typedef uint32_t  state_id_t;
    typedef uint32_t  position_t;
    static const state_id_t null_state = ~(state_id_t)0;
    static const state_id_t max_state = null_state;

    State32() {
        spos = null_state;
        minch = maxch = 0;
		term_bit = 0;
		pzip_bit = 0;
		free_bit = 0;
		IF_DUP_PATH_POOL(pool_bit = 0;)
    }

	void setch(char_t ch) { minch = maxch = ch; }
	void setMinch(char_t ch) { minch = ch; }
	void setMaxch(char_t ch) { maxch = ch; }
	char_t getMaxch() const { return maxch; }
	char_t getMinch() const { return minch; }
	bool  is_single() const { return maxch == minch; }
	bool  range_covers(char_t ch) const { return ch >= minch && ch <= maxch; }
	int   rlen()      const { return maxch - minch + 1; }
	bool  has_children() const { return minch != maxch || spos != null_state; }

    bool is_term() const { return term_bit; }
	bool is_pzip() const { return pzip_bit; }
	bool is_free() const { return free_bit; }
    void set_term_bit() { term_bit = 1; }
	void set_pzip_bit() { pzip_bit = 1; }
    void clear_term_bit() { term_bit = 0; }
	void set_free() { free_bit = 1; }
#ifdef  ENABLE_DUP_PATH_POOL
	bool is_pool() const { return pool_bit; }
	void set_pool_bit() { pool_bit = 1; }
#endif
    void setpos(size_t tpos) {
        if (tpos >= size_t(null_state) * MemPool::align_size) {
			throw std::logic_error("State32::setpos: too large tpos");
		}
        this->spos = position_t(tpos / MemPool::align_size);
    }
    size_t getpos() const {
        return size_t(spos) * MemPool::align_size;
    }
    state_id_t get_target() const { return spos; }
    void set_target(state_id_t t) { spos = t;    }

	typedef state_id_t transition_t;
	static transition_t first_trans(state_id_t t) { return t; }
}; // end class State32

/// State must has trivial destructor
template<class State>
class Automata :
	public DFA_VirtualAdapter<Automata<State> >,
	public DFA_MutationVirtualAdapter<Automata<State> >
{
	typedef typename State::transition_t Transition;
	typedef DFA_MutationVirtualAdapter<Automata<State> > mutation;
public:
	using mutation::add_move;
	class MinADFA_Module_onfly;
	class MinADFA_onfly;
	typedef State      state_t;
	typedef typename State::transition_t transition_t;
    typedef typename State::position_t position_t;
    typedef typename State::state_id_t state_id_t;
    static const  int edge_size = sizeof(Transition);
    static const  int char_bits = 8;
    static const  int sigma = 1 << char_bits; // normally is 256
    static const  state_id_t  max_state = State:: max_state;
    static const  state_id_t null_state = State::null_state;
	
    struct header_b32 {
        uint32_t   b;
        Transition s[1];
    };
    typedef bm_uint_t big_bm_b_t;
    struct header_max : public BitMap<sigma, big_bm_b_t> {
        const Transition* base(int bits) const {
            assert(bits <= this->TotalBits);
            int n = (bits + this->BlockBits-1) / this->BlockBits;
            return (const Transition*)(&this->bm[n]);
        }
        Transition* base(int bits) {
            assert(bits <= this->TotalBits);
            int n = (bits + this->BlockBits-1) / this->BlockBits;
            return (Transition*)(&this->bm[n]);
        }
    };

    static int bm_real_bits(int n) {
        if (is_32bitmap(n))
            return 32;
        else // align to blockBits boundary
            return header_max::align_bits(n);
    }

	static bool is_32bitmap(int bits) {
		assert(bits >= 2);
		return sizeof(Transition) <= 4 &&
		       sizeof(big_bm_b_t) >  4 &&
			   bits <= 32;
	}

	bool has_freelist() const override final { return true; }

private:
    void insert(State& s, Transition val,
                int oldbits, int newbits, int inspos, int oldouts) {
        assert(oldbits >= 32);
        assert(oldbits <= newbits);
        assert(oldbits <= sigma);
        assert(newbits <= sigma);
        assert(oldbits == 32 || oldbits % header_max::BlockBits == 0);
        assert(newbits == 32 || newbits % header_max::BlockBits == 0);
        assert(oldouts <= oldbits);
        assert(inspos >= 0);
        assert(inspos <= oldouts);
        size_t oldpos = s.getpos();
        assert(oldpos < pool.size());
        int cbOldBmp = oldbits / 8;
        int cbNewBmp = newbits / 8;
        int oldTotal = pool.align_to(cbOldBmp + (oldouts + 0) * edge_size);
        int newTotal = pool.align_to(cbNewBmp + (oldouts + 1) * edge_size);
        // after align, oldTotal may equal to newTotal
        assert(oldpos + oldTotal <= pool.size());
        if (pool.size() == oldpos + oldTotal || oldTotal == newTotal) {
            if (oldTotal != newTotal)
                pool.resize_no_init(oldpos + newTotal);
            char* me = &pool.at<char>(oldpos);
            if (inspos != oldouts)
                // in-place need backward copy, use memmove
                memmove(me + cbNewBmp + (inspos + 1) * edge_size,
                        me + cbOldBmp + (inspos + 0) * edge_size,
                        (oldouts - inspos) * edge_size); // copy post-inspos
            ((Transition*)(me + cbNewBmp))[inspos] = val;
            if (cbOldBmp != cbNewBmp) {
                memmove(me + cbNewBmp,
                        me + cbOldBmp, // copy pre-inspos
                        inspos * edge_size);
                memset(me + cbOldBmp, 0, cbNewBmp-cbOldBmp);
            }
        } else { // is not in-place copy, memcpy is ok
            size_t newpos = pool.alloc(newTotal);
            assert(newpos != oldpos);
            char* base = &pool.at<char>(0);
            if (cbOldBmp == cbNewBmp) {
                memcpy(base + newpos,
                       base + oldpos, // bitmap + pre-inspos
                       cbOldBmp + inspos * edge_size);
            } else {
                memcpy(base + newpos, base + oldpos, cbOldBmp); // copy bitmap
                memset(base + newpos + cbOldBmp, 0, cbNewBmp-cbOldBmp);
                memcpy(base + newpos + cbNewBmp,
                       base + oldpos + cbOldBmp,
                       inspos * edge_size); // copy pre-inspos
            }
            ((Transition*)(base + newpos + cbNewBmp))[inspos] = val;
            memcpy(base + newpos + cbNewBmp + (inspos + 1) * edge_size,
                   base + oldpos + cbOldBmp + (inspos + 0) * edge_size,
                   (oldouts - inspos) * edge_size); // copy post-inspos
            pool.sfree(oldpos, oldTotal);
            s.setpos(newpos);
        }
    }

    void check_slots(State s, int n) {
#if defined(NDEBUG)
		(void)s;
		(void)n;
#else
        int bits = bm_real_bits(s.rlen());
        const Transition* p = &pool.at<Transition>(s.getpos() + bits/8);
        if (is_32bitmap(bits)) {
            const header_b32& hb = pool.at<header_b32>(s.getpos());
            int n2 = fast_popcount32(hb.b);
            assert(n2 == n);
        } else {
            const header_max& hb = pool.at<header_max>(s.getpos());
            int n2 = hb.popcnt_aligned(bits);
            assert(n2 == n);
        }
        for (int i = 0; i < n; ++i) {
        //  assert(p[i] >= 0); // state_id_t is unsigned
			ASSERT_isNotFree(p[i]);
			assert(p[i] < (state_id_t)states.size());
        }
#endif
    }

	void init() {
        states.push_back(State()); // initial_state
        firstFreeState = null_state;
        numFreeStates  = 0;
        transition_num = 0;
		zpath_states = 0;
		IF_DUP_PATH_POOL(allow_zpath_pool = false;)
	}

public:

    Automata() : pool(sizeof(Transition)*sigma + sigma/8) { init(); }
	void clear() { states.clear(); pool.clear(); init(); }
	void erase_all() { states.erase_all(); pool.erase_all(); init(); }

	const valvec<State>& internal_get_states() const { return states; }
	const MemPool      & internal_get_pool  () const { return pool  ; }
    size_t total_states() const { return states.size(); }
    size_t total_transitions() const { return transition_num; }
    size_t new_state() override final {
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
    size_t clone_state(size_t source) override final {
        assert(source < states.size());
        size_t y = new_state();
        const State s = states[source];
		states[y] = s;
        if (!s.is_single())  {
			int bits = s.rlen();
			size_t nb;
			if (is_32bitmap(bits)) {
				int trans = fast_popcount32(pool.at<header_b32>(s.getpos()).b);
				nb = 4 + sizeof(Transition) * trans;
				transition_num += trans;
			} else {
				const header_max& hb = pool.at<header_max>(s.getpos());
				int trans = hb.popcnt_aligned(bits);
				nb = hb.align_bits(bits)/8 + sizeof(Transition) * trans;
				transition_num += trans;
			}
			size_t pos = pool.alloc(pool.align_to(nb));
			states[y].setpos(pos);
			memcpy(&pool.at<char_t>(pos), &pool.at<char_t>(s.getpos()), nb);
        }
		else if (s.is_pzip()) {
			// very unlikely:
			assert(0); // now, unreachable
			int nzc = pool.at<byte_t>(sizeof(state_id_t) + s.getpos());
			size_t nb = sizeof(state_id_t) + nzc + 1;
			size_t pos = pool.alloc(pool.align_to(nb));
			memcpy(&pool.at<char_t>(pos), &pool.at<char_t>(s.getpos()), nb);
			states[y].setpos(pos);
		}
		else if (null_state != s.get_target()) {
			transition_num += 1;
		}
		return y;
    }
    void del_state(size_t s) override final {
		assert(0 == zpath_states);
        assert(s < states.size());
        assert(s != initial_state); // initial_state shouldn't be deleted
		State& x = states[s];
		assert(!x.is_pzip()); // must be non-path-zipped
		if (!x.is_single()) { // has transitions in pool
			int bits = x.rlen();
            size_t pos = x.getpos();
            size_t bytes;
            if (is_32bitmap(bits)) {
                uint32_t bm = pool.at<uint32_t>(pos);
				int trans = fast_popcount32(bm);
                bytes = 4 +  trans * sizeof(Transition);
				transition_num -= trans;
            } else {
				int trans = pool.at<header_max>(pos).popcnt_aligned(bits);
                bytes = trans * sizeof(Transition)
                      + header_max::align_bits(bits) / 8;
				transition_num -= trans;
            }
            pool.sfree(pos, pool.align_to(bytes));
        }
	   	else if (x.get_target() != null_state)
			transition_num -= 1; // just 1 transition
		// put s to free list, states[s] is not a 'State' from now
		x.set_target(firstFreeState);
		x.set_free();
		firstFreeState = s;
		numFreeStates++;
    }
	void del_all_children(state_id_t s) {
		assert(0 == zpath_states);
        assert(s < states.size());
		State& x = states[s];
		assert(!x.is_pzip()); // must be non-path-zipped
		if (!x.is_single()) { // has transitions in pool
			int bits = x.rlen();
            size_t pos = x.getpos();
            size_t bytes;
            if (is_32bitmap(bits)) {
                uint32_t bm = pool.at<uint32_t>(pos);
				int trans = fast_popcount32(bm);
                bytes = 4 +  trans * sizeof(Transition);
				transition_num -= trans;
            } else {
				int trans = pool.at<header_max>(pos).popcnt_aligned(bits);
                bytes = trans * sizeof(Transition)
                      + header_max::align_bits(bits) / 8;
				transition_num -= trans;
            }
            pool.sfree(pos, pool.align_to(bytes));
        }
	   	else if (x.get_target() != null_state)
			transition_num -= 1; // just 1 transition
		x.set_target(null_state);
		x.setch(0);
	}
	state_id_t min_free_state() const {
		state_id_t s = firstFreeState;
		state_id_t s_min = states.size()-1;
		while (null_state != s) {
			s_min = std::min(s, s_min);
			s = states[s].get_target();
		}
		return s_min;
	}
	size_t num_free_states() const { return numFreeStates; }
	size_t num_used_states() const { return states.size() - numFreeStates; }

    bool is_free(state_id_t s) const {
        assert(size_t(s) < states.size());
        return states[s].is_free();
    }
	bool is_pzip(state_id_t s) const {
		return states[s].is_pzip();
	}
    bool is_term(state_id_t s) const {
        assert(s < (state_id_t)states.size());
        return states[s].is_term();
    }
    void set_term_bit(state_id_t s) {
        assert(s < (state_id_t)states.size());
        states[s].set_term_bit();
    }
    void clear_term_bit(state_id_t s) {
        assert(s < (state_id_t)states.size());
        states[s].clear_term_bit();
    }

	bool has_children(state_id_t s) const {
        assert(s < (state_id_t)states.size());
		return states[s].has_children();
	}
	bool has_single_child(state_id_t s) const {
        assert(s < (state_id_t)states.size());
		return states[s].is_single();
	}
	void set_single_child(state_id_t parent, state_id_t child) {
		assert(0 == zpath_states);
        assert(parent < (state_id_t)states.size());
        assert(child  < (state_id_t)states.size());
		states[parent].set_target(child);
	}
	state_id_t get_single_child(state_id_t parent) const {
		assert(0 == zpath_states);
        assert(parent < (state_id_t)states.size());
		return states[parent].get_target();
	}
	byte_t get_single_child_char(state_id_t parent) const {
		assert(0 == zpath_states);
        assert(parent < (state_id_t)states.size());
		return states[parent].getMinch();
	}

    Transition state_move(state_id_t curr, char_t ch) const {
        assert(curr < (state_id_t)states.size());
		ASSERT_isNotFree(curr);
        return state_move(states[curr], ch);
	}
    Transition state_move(const State& s, char_t ch) const {
        assert(s.getMinch() <= s.getMaxch());
        if (!s.range_covers(ch))
            return null_state;
        if (s.is_single())
			return State::first_trans(single_target(s));
        int bpos = ch - s.getMinch(); // bit position
        int bits = s.rlen();
        if (is_32bitmap(bits)) {
            const header_b32& hb = pool.at<header_b32>(s.getpos());
            if (hb.b & uint32_t(1)<<bpos) {
                int index = fast_popcount_trail(hb.b, bpos);
                return hb.s[index];
            }
        } else {
            const header_max& hb = pool.at<header_max>(s.getpos());
            if (hb.is1(bpos)) {
                int index = hb.popcnt_index(bpos);
                return hb.base(bits)[index];
            }
        }
        return null_state;
    }
	void del_move(state_id_t s, char_t ch) {
		BitMap<sigma, bm_uint_t> bits(0);
		state_id_t children[sigma];
	//	printf("del_move------------: c=%c:%02X\n", ch, ch);
		{
			ptrdiff_t pos = 0;
			for_each_move(s, [&](state_id_t, state_id_t t, char_t c1) {
				if (c1 != ch) {
					assert(pos < sigma);
					bits.set1(c1);
					children[pos++] = t;
				}
	//			printf("del_move_loop: c=%c:%02X, pos=%zd\n", c1, c1, pos);
			});
		}
		del_all_children(s);
		for (ptrdiff_t i = 0, pos = 0; i < bits.BlockN; ++i) {
			bm_uint_t bm = bits.block(i);
	//		printf("add_move_loop: block(%ld)=%0lX\n", i, bm);
			for (char_t c2 = i * bits.BlockBits; bm; pos++) {
				int ctz = fast_ctz(bm);
				c2 += ctz;
				add_move(s, children[pos], c2);
	//			printf("add_move_loop: c=%c:%02X, pos=%zd\n", c2, c2, pos);
				c2++;
				bm >>= ctz;
				bm >>= 1;
			}
		}
	}
protected:
    size_t add_move_imp(size_t source, size_t target, char_t ch, bool OverwriteExisted) override final {
        assert(source < states.size());
        assert(target < states.size());
		ASSERT_isNotFree(source);
		ASSERT_isNotFree(target);
		assert(!states[source].is_pzip());
	//	assert(!states[target].is_pzip()); // target could be path zipped
        State& s = states[source];
        assert(s.getMinch() <= s.getMaxch());
        if (s.is_single()) {
			const size_t old_target = s.get_target();
            if (null_state == old_target) {
            	assert(0 == s.getMinch()); // temporary assert
                s.setch(ch);
                s.set_target(target);
				transition_num++;
				return null_state;
            }
            if (s.getMinch() == ch) {
                // the target for ch has existed
				if (OverwriteExisted)
					s.set_target(target);
                return old_target;
            }
			char_t minch = s.getMinch();
			char_t maxch = minch;
			assert(0 != minch); // temporary assert
            // 's' was direct link to the single target state
            // now alloc memory in pool
            if  (s.getMinch() > ch)
                 minch = ch, s.setMinch(ch);
            else maxch = ch, s.setMaxch(ch);
            int bits = bm_real_bits(s.rlen());
            size_t pos = pool.alloc(pool.align_to(bits/8 + 2 * edge_size));
            if (is_32bitmap(bits)) {
                // lowest bit must be set
                pool.at<uint32_t>(pos) = 1 | uint32_t(1) << (maxch - minch);
            } else {
                memset(&pool.at<char>(pos), 0, bits/8);
                pool.at<header_max>(pos).set1(0);
                pool.at<header_max>(pos).set1(maxch - minch);
            }
            Transition* trans = &pool.at<Transition>(pos + bits/8);
            if (ch == minch) {
                trans[0] = target;
                trans[1] = State::first_trans(old_target);
            } else {
                trans[0] = State::first_trans(old_target);
                trans[1] = target;
            }
            s.setpos(pos);
            check_slots(s, 2);
			transition_num++;
            return null_state;
        }
        assert(null_state != s.get_target());
        int oldbits = bm_real_bits(s.rlen());
        int oldouts;
        if (ch < s.getMinch()) {
            int newbits = bm_real_bits(s.getMaxch() - ch + 1);
            if (is_32bitmap(newbits)) { // oldbits also == 32
                header_b32& hb = pool.at<header_b32>(s.getpos());
                oldouts = fast_popcount32(hb.b);
                hb.b <<= s.getMinch() - ch;
                hb.b  |= 1; // insert target at beginning
                insert(s, target, oldbits, newbits, 0, oldouts);
            } else { // here newbits > 32...
                header_max* hb;
                if (is_32bitmap(oldbits)) {
                    uint32_t oldbm32 = pool.at<header_b32>(s.getpos()).b;
                    oldouts = fast_popcount32(oldbm32);
                    insert(s, target, oldbits, newbits, 0, oldouts);
                    hb = &pool.at<header_max>(s.getpos());
                    hb->block(0) = oldbm32; // required for bigendian CPU
                } else {
                    oldouts = pool.at<header_max>(s.getpos()).popcnt_aligned(oldbits);
                    insert(s, target, oldbits, newbits, 0, oldouts);
                    hb = &pool.at<header_max>(s.getpos());
                }
                hb->shl(s.getMinch() - ch, newbits/hb->BlockBits);
                hb->set1(0);
            }
            s.setMinch(ch);
            check_slots(s, oldouts+1);
        } else if (ch > s.getMaxch()) {
            int newbits = bm_real_bits(ch - s.getMinch() + 1);
            if (is_32bitmap(newbits)) { // oldbits is also 32
                header_b32& hb = pool.at<header_b32>(s.getpos());
                oldouts = fast_popcount32(hb.b);
                hb.b |= uint32_t(1) << (ch - s.getMinch());
                insert(s, target, oldbits, newbits, oldouts, oldouts);
            } else { // now newbits > 32
                header_max* hb;
                if (is_32bitmap(oldbits)) {
                    uint32_t oldbm32 = pool.at<header_b32>(s.getpos()).b;
                    oldouts = fast_popcount32(oldbm32);
                    insert(s, target, oldbits, newbits, oldouts, oldouts);
                    hb = &pool.at<header_max>(s.getpos());
                    hb->block(0) = oldbm32; // required for bigendian CPU
                } else {
                    oldouts = pool.at<header_max>(s.getpos()).popcnt_aligned(oldbits);
                    insert(s, target, oldbits, newbits, oldouts, oldouts);
                    hb = &pool.at<header_max>(s.getpos());
                }
				hb->set1(ch - s.getMinch());
            }
            s.setMaxch(ch);
            check_slots(s, oldouts+1);
        } else { // ch is in [s.minch, s.maxch], doesn't need to expand bitmap
            int inspos;
            int bitpos = ch - s.getMinch();
            if (is_32bitmap(oldbits)) {
                header_b32& hb = pool.at<header_b32>(s.getpos());
                oldouts = fast_popcount32(hb.b);
                inspos = fast_popcount_trail(hb.b, bitpos);
                if (hb.b & uint32_t(1) << bitpos) {
					state_id_t old = hb.s[inspos];
                    if (OverwriteExisted)
						hb.s[inspos] = target;
                    return old;
                }
                hb.b |= uint32_t(1) << bitpos;
            } else {
                header_max& hb = pool.at<header_max>(s.getpos());
                oldouts = hb.popcnt_aligned(oldbits);
                inspos = hb.popcnt_index(bitpos);
                if (hb.is1(bitpos)) {
					state_id_t old = hb.base(oldbits)[inspos];
                    if (OverwriteExisted)
					   	hb.base(oldbits)[inspos] = target;
					return old;
                }
                hb.set1(bitpos);
            }
            insert(s, target, oldbits, oldbits, inspos, oldouts);
            check_slots(s, oldouts+1);
        }
        transition_num++;
		return null_state;
    }
public:
	size_t trans_num(state_id_t curr) const {
        const State s = states[curr];
        if (s.is_single()) {
            state_id_t target = single_target(s);
			return (null_state == target) ? 0 : 1;
        }
        int bits = s.rlen(), tn;
        if (is_32bitmap(bits)) {
            const header_b32& hb = pool.at<header_b32>(s.getpos());
            tn = fast_popcount32(hb.b);
        } else {
            const header_max& hb = pool.at<header_max>(s.getpos());
            tn = hb.popcnt_aligned(bits);
        }
		return tn;
	}

	size_t trans_bytes(state_id_t sid) const {
		return trans_bytes(states[sid]);
	}
	size_t trans_bytes(const State& s) const {
        if (s.is_single()) {
			assert(s.is_pzip());
			return sizeof(state_id_t);
		}
        int bits = s.rlen();
        if (is_32bitmap(bits)) {
            const header_b32& hb = pool.at<header_b32>(s.getpos());
			int tn = fast_popcount32(hb.b);
			return tn * sizeof(Transition) + 4;
        } else {
            const header_max& hb = pool.at<header_max>(s.getpos());
            int tn = hb.popcnt_aligned(bits);
			return tn * sizeof(Transition) + hb.align_bits(bits)/8;
        }
	}

	state_id_t single_target(state_id_t s) const { return single_target(states[s]); }
	state_id_t single_target(const State& s) const {
		// just 1 trans, if not ziped, needn't extra memory in pool
		// default single_target is null_state, just the designated
		return s.is_pzip() ? pool.at<state_id_t>(s.getpos()) : s.get_target();
	}

    // just walk the (curr, dest), don't access the 'ch'
    // this is useful such as when minimizing DFA
    template<class OP> // use ctz (count trailing zero)
    void for_each_dest(state_id_t curr, OP op) const {
        const State& s = states[curr];
        if (s.is_single()) {
            state_id_t target = single_target(s);
            if (null_state != target)
                op(curr, State::first_trans(target));
            return;
        }
        int bits = s.rlen();
        int tn;
        const Transition* tp;
        if (is_32bitmap(bits)) {
            const header_b32& hb = pool.at<header_b32>(s.getpos());
            tn = fast_popcount32(hb.b);
            tp = hb.s;
        } else {
            const header_max& hb = pool.at<header_max>(s.getpos());
            tn = hb.popcnt_aligned(bits);
            tp = hb.base(bits);
        }
        for (int i = 0; i < tn; ++i)
            op(curr, tp[i]);
    }

	// for_each_dest in reversed order
    template<class OP>
    void for_each_dest_rev(state_id_t curr, OP op) const {
        const State& s = states[curr];
        if (s.is_single()) {
            state_id_t target = single_target(s);
            if (null_state != target)
                op(curr, State::first_trans(target));
            return;
        }
        int bits = s.rlen();
        int tn;
        const Transition* tp;
        if (is_32bitmap(bits)) {
            const header_b32& hb = pool.at<header_b32>(s.getpos());
            tn = fast_popcount32(hb.b);
            tp = hb.s;
        } else {
            const header_max& hb = pool.at<header_max>(s.getpos());
            tn = hb.popcnt_aligned(bits);
            tp = hb.base(bits);
        }
        for (int i = tn-1; i >= 0; --i)
            op(curr, tp[i]);
    }

    template<class OP> // use ctz (count trailing zero)
    void for_each_move(state_id_t curr, OP op) const {
		ASSERT_isNotFree(curr);
        const State s = states[curr];
        if (s.is_single()) {
			state_id_t target = single_target(s);
			if (null_state != target)
				op(curr, State::first_trans(target), s.getMinch());
			return;
		}
        const int bits = s.rlen();
        if (is_32bitmap(bits)) {
            const header_b32& hb = pool.at<header_b32>(s.getpos());
            uint32_t bm = hb.b;
            char_t   ch = s.getMinch();
            for (int pos = 0; bm; ++pos) {
                const Transition& target = hb.s[pos];
                int ctz = fast_ctz32(bm);
                ch += ctz;
                op(curr, target, ch);
                ch++;
                bm >>= ctz; // must not be bm >>= ctz + 1
                bm >>= 1;   // because ctz + 1 may be bits of bm(32 or 64)
            }
        } else {
            const header_max& hb = pool.at<header_max>(s.getpos());
            const Transition* base = hb.base(bits);
            const int cBlock = hb.align_bits(bits) / hb.BlockBits;
            for (int pos = 0, i = 0; i < cBlock; ++i) {
                big_bm_b_t bm = hb.block(i);
                char_t ch = s.getMinch() + i * hb.BlockBits;
                for (; bm; ++pos) {
                    const Transition& target = base[pos];
                    int ctz = fast_ctz(bm);
                    ch += ctz;
                    op(curr, target, ch);
                    ch++;
                    bm >>= ctz; // must not be bm >>= ctz + 1
                    bm >>= 1;   // because ctz + 1 may be bits of bm(32 or 64)
                    assert(pos < hb.BlockBits * cBlock);
                }
            }
        }
    }

	// exit immediately if op returns false
    template<class OP> // use ctz (count trailing zero)
    void for_each_move_break(state_id_t curr, OP op) const {
		ASSERT_isNotFree(curr);
        const State s = states[curr];
        if (s.is_single()) {
			state_id_t target = single_target(s);
			if (null_state != target)
				op(curr, State::first_trans(target), s.getMinch());
			return;
		}
        const int bits = s.rlen();
        if (is_32bitmap(bits)) {
            const header_b32& hb = pool.at<header_b32>(s.getpos());
            uint32_t bm = hb.b;
            char_t   ch = s.getMinch();
            for (int pos = 0; bm; ++pos) {
                const Transition& target = hb.s[pos];
                int ctz = fast_ctz32(bm);
                ch += ctz;
                if (!op(curr, target, ch))
					return;
                ch++;
                bm >>= ctz; // must not be bm >>= ctz + 1
                bm >>= 1;   // because ctz + 1 may be bits of bm(32 or 64)
            }
        } else {
            const header_max& hb = pool.at<header_max>(s.getpos());
            const Transition* base = hb.base(bits);
            const int cBlock = hb.align_bits(bits) / hb.BlockBits;
            for (int pos = 0, i = 0; i < cBlock; ++i) {
                big_bm_b_t bm = hb.block(i);
                char_t ch = s.getMinch() + i * hb.BlockBits;
                for (; bm; ++pos) {
                    const Transition& target = base[pos];
                    int ctz = fast_ctz(bm);
                    ch += ctz;
                    if (!op(curr, target, ch))
						return;
                    ch++;
                    bm >>= ctz; // must not be bm >>= ctz + 1
                    bm >>= 1;   // because ctz + 1 may be bits of bm(32 or 64)
                    assert(pos < hb.BlockBits * cBlock);
                }
            }
        }
    }

    template<class OP>
    void for_each_move_slow(state_id_t curr, OP op) const {
		ASSERT_isNotFree(curr);
        const State s = states[curr];
        if (s.is_single()) {
            state_id_t target = single_target(s);
            if (null_state != target)
                op(curr, State::first_trans(target), s.getMinch());
			return;
        }
        const int bits = s.rlen();
        if (is_32bitmap(bits)) {
            const header_b32& hb = pool.at<header_b32>(s.getpos());
            uint32_t bm = hb.b;
            char_t   ch = s.getMinch();
            for (int pos = 0; bm; ++ch) {
                if (bm & 1) {
                    const Transition& target = hb.s[pos];
                    op(curr, target, ch);
                    pos++;
                }
                bm >>= 1;
            }
        } else {
            const header_max& hb = pool.at<header_max>(s.getpos());
            const state_id_t* base = hb.base(bits);
            const int cBlock = hb.align_bits(bits) / hb.BlockBits;
            for (int pos = 0, i = 0; i < cBlock; ++i) {
                big_bm_b_t bm = hb.block(i);
                // could use intrinsic ctz(count of trailing zero)
                // to reduce time complexity
                char_t ch = s.getMinch() + i * hb.BlockBits;
                for (; bm; ++ch) {
                    if (bm & 1) {
                        const Transition& target = base[pos];
                        op(curr, target, ch);
                        if (ch >= s.getMaxch()) // additional optimization
                            return;
                        pos++;
                    }
                    bm >>= 1;
                }
            }
        }
    }
	
	typedef BFS_GraphWalker<state_id_t>  bfs_walker;
	typedef DFS_GraphWalker<state_id_t>  dfs_walker;
	typedef PFS_GraphWalker<state_id_t>  pfs_walker;

    void resize_states(size_t new_states_num) {
		if (new_states_num >= max_state)
			throw std::logic_error("Automata::resize_states: exceed max_state");
        states.resize(new_states_num);
    }

    size_t mem_size() const override {
        return pool.size() + sizeof(State) * states.size();
    }
	size_t free_size() const { return pool.free_size(); }

	void shrink_to_fit() {
		states.shrink_to_fit();
		pool.shrink_to_fit();
	}

    void swap(Automata& y) {
        pool  .swap(y.pool  );
        states.swap(y.states);
		std::swap(firstFreeState, y.firstFreeState);
		std::swap(numFreeStates , y.numFreeStates );
        std::swap(transition_num, y.transition_num);
        std::swap(zpath_states  , y.zpath_states  );
    }

#ifdef ENABLE_DUP_PATH_POOL
	struct ZpoolEntry {
		size_t cnt;
		size_t pos;
		const static size_t null_pos = 1;
		ZpoolEntry() : cnt(0), pos(null_pos) {}
	};
#endif
	template<class BaseWalker>
	class ZipWalker : public BaseWalker {
	public:
        valvec<state_id_t> path; // state_move(path[j], strp[j]) == path[j+1]
        valvec<char_t>     strp;

		void getOnePath(const Automata* au, const bit_access is_confluence) {
			const valvec<State>& states = au->internal_get_states();
			state_id_t s = this->next();
			assert(!states[s].is_pzip());
			strp.resize0();
            path.resize0();
            path.push_back(s);
            if (states[s].is_term() || !states[s].is_single())
				return;
			char_t prev_ch = states[s].getMinch();
			for (state_id_t i = states[s].get_target(); // single target
				 path.size() < 255 && !is_confluence[i]; ) {
				ASSERT_isNotFree2(au, i);
				assert(!states[i].is_pzip());
				assert(this->color.is0(i));
			// 	this->color.set1(i); // not needed
				strp.push_back(prev_ch);
				path.push_back(i);
				if (states[i].is_term() || !states[i].is_single()) break;
				prev_ch = states[i].getMinch();
				i = states[i].get_target();
			// only term state could have null target:
				assert(i != null_state);
				ASSERT_isNotFree2(au, i);
			}
			strp.push_back('\0');
			strp.unchecked_pop_back();
		 // state_move(path[j], strp[j]) == path[j+1]:
			assert(path.size() == strp.size() + 1);
		}
	};
    
	///@param BFS_or_DFS "bfs" or "dfs"
	void path_zip(const char* BFS_or_DFS) {
		Automata zipped;
		path_zip(zipped, BFS_or_DFS);
		this->swap(zipped);
	}
    template<class ZippedAutomata>
    void path_zip(ZippedAutomata& zipped, const char* BFS_or_DFS) const {
		simplebitvec is_confluence(states.size(), false);
		{
			valvec<state_id_t> indg;
			dfa_algo(this).compute_indegree(initial_state, indg);
			for (size_t i = 0, n = indg.size(); i < n; ++i) {
				if (indg[i] > 1)
					is_confluence.set1(i);
			}
		}
		path_zip(is_confluence.risk_data(), zipped, BFS_or_DFS);
	}
    template<class ZippedAutomata>
    void path_zip(const bit_access is_confluence,
			  ZippedAutomata& zipped, const char* BFS_or_DFS) const {
		valvec<typename ZippedAutomata::state_id_t> s2ds;
		if (strcasecmp(BFS_or_DFS, "BFS") == 0)
			path_zip<ZippedAutomata, bfs_walker>(is_confluence, zipped, s2ds);
		else if (strcasecmp(BFS_or_DFS, "DFS") == 0)
			path_zip<ZippedAutomata, dfs_walker>(is_confluence, zipped, s2ds);
		else if (strcasecmp(BFS_or_DFS, "PFS") == 0)
			path_zip<ZippedAutomata, pfs_walker>(is_confluence, zipped, s2ds);
		else
			throw std::invalid_argument("Automata::path_zip: must be one of: BFS, DFS, PFS");
	}
    template<class ZippedAutomata, class BaseWalker>
    void path_zip(const bit_access is_confluence, ZippedAutomata& zipped,
				  valvec<typename ZippedAutomata::state_id_t>& s2ds) const {
		assert(0 == zpath_states);
		ASSERT_isNotFree(initial_state);
		zipped.erase_all();
        const size_t min_compress_path_len = 3;
#if defined(NDEBUG)
	   	s2ds.resize_no_init(states.size());
#else
	   	s2ds.resize(states.size(), ZippedAutomata::null_state+0);
#endif
		IF_DUP_PATH_POOL(hash_strmap<ZpoolEntry> zwc;)
		ZipWalker<BaseWalker> walker;
		{	size_t ds = 0;
			walker.resize(states.size());
			walker.put_root(initial_state);
			while (!walker.is_finished()) {
				walker.getOnePath(this, is_confluence);
				size_t n = walker.path.size();
				if (n >= min_compress_path_len) {
					for (size_t j = 0; j < n; ++j) s2ds[walker.path[j]] = ds;
					ds++;
#ifdef ENABLE_DUP_PATH_POOL
					if (zipped.allow_zpath_pool && n >= min_compress_path_len + 2) {
						fstring w((const char*)&walker.strp[0], walker.strp.size());
						zwc[w].cnt++;
					}
#endif //---------------------------------
				} else
					for (size_t j = 0; j < n; ++j) s2ds[walker.path[j]] = ds++;
				walker.putChildren(this, walker.path.back());
			}
			zipped.resize_states(ds);
	   	}
		IF_DUP_PATH_POOL(zwc.sort_fast();)
		walker.resize(states.size());
		walker.put_root(initial_state);
		size_t n_zpath_states = 0;
        while (!walker.is_finished()) {
			walker.getOnePath(this, is_confluence);
			typename ZippedAutomata::state_id_t zs = s2ds[walker.path.back()];
			this->for_each_move(walker.path.back(),
			  [&](state_id_t, state_id_t t, char_t c) {
				zipped.add_move_checked(zs, s2ds[t], c);
			});
            if (walker.path.size() >= min_compress_path_len) {
				n_zpath_states++;
#ifdef ENABLE_DUP_PATH_POOL
				if (zipped.allow_zpath_pool 
						&& walker.path.size() >= min_compress_path_len + 2) {
					fstring w((const char*)&walker.strp[0], walker.strp.size());
					size_t idx = zwc.find_i(w);
					assert(idx < zwc.size());
					ZpoolEntry& ze = zwc.val(idx);
					if (ZpoolEntry::null_pos == ze.pos) {
						size_t pos = zipped.add_zpath(zs, walker.strp);
						assert(pos % sizeof(state_id_t) == 0);
						if (ze.cnt >= 2) {
							ze.pos = pos;
						//	printf("zip:cnt=%04zd:%.*s\n", ze.cnt, (int)w.n, w.p);
						}
					} else
						zipped.add_zpool(zs, ze.pos);
				} else
#endif //--------------------------------------------------
					zipped.add_zpath(zs, walker.strp);
            } else {
				const state_id_t* path = &walker.path[0];
                for (size_t j = 0; j < walker.strp.size(); ++j)
                    zipped.add_move_checked(s2ds[path[j]], s2ds[path[j+1]], walker.strp[j]);
            }
			if (states[walker.path.back()].is_term())
				zipped.set_term_bit(zs);
			walker.putChildren(this, walker.path.back());
		}
	#if 0 // && defined(ENABLE_DUP_PATH_POOL)
		FILE* fp = fopen("zipped_path.txt", "w");
		if (fp) {
			size_t total = 0;
			for (size_t i = 0; i < zwc.size(); ++i) {
				auto w = zwc.key(i);
				auto cnt = zwc.val(i).cnt;
			//	auto pos = zwc.val(i).pos;
				size_t bytes = w.n * cnt;
				fprintf(fp, "%04zd\t:%s\t%zd\t%zd\n", cnt, w.p, w.n, bytes);
				if (cnt >= 4)
					total += bytes + cnt;
			}
			fprintf(fp, "total zippable=%zd\n", total);
			fclose(fp);
		}
	#endif
		zipped.path_zip_freeze(n_zpath_states);
	}

	void path_zip_freeze(size_t n_zpath_states) {
   		pool.shrink_to_fit();
		zpath_states = n_zpath_states;
	}

	size_t num_zpath_states() const { return zpath_states; }

    size_t add_zpath(state_id_t ss, const valvec<char_t>& str) {
		return add_zpath(ss, &str[0], str.size());
	}
    size_t add_zpath(state_id_t ss, const char_t* str, size_t len) {
        assert(len <= 255);
		assert(!states[ss].is_pzip());
		char_t* p;
		size_t  newpos;
		if (states[ss].is_single()) {
			newpos = pool.alloc(pool.align_to(sizeof(state_id_t) + len+1));
			pool.at<state_id_t>(newpos) = states[ss].get_target();
			p = &pool.at<char_t>(newpos + sizeof(state_id_t));
		}
		else {
			size_t oldpos = states[ss].getpos();
			size_t oldlen = trans_bytes(ss);
			size_t newlen = pool.align_to(oldlen + len + 1);
			newpos = pool.alloc3(oldpos, pool.align_to(oldlen), newlen);
			p = &pool.at<char_t>(newpos + oldlen);
		}
		states[ss].setpos(newpos);
		p[0] = len;
		memcpy(p+1, str, len);
		states[ss].set_pzip_bit();
		return p - &pool.at<char_t>(0);
    }

#ifdef ENABLE_DUP_PATH_POOL
	void add_zpool(state_id_t ss, size_t zpos) {
		assert(!states[ss].is_pzip());
		assert(zpos % sizeof(state_id_t) == 0);
		size_t newpos;
		if (states[ss].is_single()) {
			newpos = pool.alloc(pool.align_to(sizeof(state_id_t) * 2));
			pool.at<state_id_t>(newpos) = states[ss].get_target();
			pool.at<state_id_t>(newpos + sizeof(state_id_t)) = zpos / sizeof(state_id_t);
		}
		else {
			size_t oldpos = states[ss].getpos();
			size_t oldlen = trans_bytes(ss);
			size_t newlen = pool.align_to(oldlen + sizeof(state_id_t));
			newpos = pool.alloc3(oldpos, pool.align_to(oldlen), newlen);
			pool.at<state_id_t>(newpos + oldlen) = zpos / sizeof(state_id_t);
		}
		states[ss].setpos(newpos);
		states[ss].set_pzip_bit();
		states[ss].set_pool_bit();
	}
#endif //--------------------------

	size_t get_zipped_path_str_pos(const State& s) const {
		assert(s.is_pzip());
		size_t xpos;
		if (s.is_single())
			// before xpos, it is the single_target
			xpos = s.getpos() + sizeof(state_id_t);
		else
			xpos = s.getpos() + trans_bytes(s);
#ifdef ENABLE_DUP_PATH_POOL
		if (s.is_pool()) {
			size_t strpos = pool.at<state_id_t>(xpos);
			strpos *= sizeof(state_id_t);
			return strpos;
		} else
#endif //////////////////////
			return xpos;
	}
public:
	const byte_t* get_zpath_data(const State& s) const {
		assert(!s.is_free());
		size_t zpos = get_zipped_path_str_pos(s);
		return pool.data() + zpos;
	}
	const byte_t* get_zpath_data(size_t s) const {
		assert(s < states.size());
		assert(!states[s].is_free());
		size_t zpos = get_zipped_path_str_pos(states[s]);
		return pool.data() + zpos;
	}

	IF_DUP_PATH_POOL(bool   allow_zpath_pool;)

	enum { SERIALIZATION_VERSION = 3 };

	template<class DataIO> void load_au(DataIO& dio, size_t version) {
		typename DataIO::my_var_uint64_t n_pool, n_states;
		typename DataIO::my_var_uint64_t n_firstFreeState, n_numFreeStates;
		typename DataIO::my_var_uint64_t n_zpath_states;
		dio >> n_zpath_states;
		dio >> n_pool; // keeped but not used in version=2
		dio >> n_states;
		dio >> n_firstFreeState;
		dio >> n_numFreeStates;
		zpath_states = n_zpath_states;
		states.resize_no_init(n_states.t);
		if (1 == version) {
			pool.resize_no_init(n_pool.t);
			if (pool.size())
				dio.ensureRead(&pool.template at<byte_t>(0), n_pool.t);
		}
		else {
			if (version > 3) {
				typedef typename DataIO::my_BadVersionException bad_ver;
				throw bad_ver(version, SERIALIZATION_VERSION, "Automata");
			}
			dio >> pool;
		}
		dio.ensureRead(&states[0], sizeof(State)*n_states.t);
		if (version < 3 && zpath_states) {
			zpath_states = 0;
			for (size_t i = 0; i < n_states.t; ++i) {
				if (!states[i].is_free() && states[i].is_pzip())
					zpath_states++;
			}
		}
	}
	template<class DataIO> void save_au(DataIO& dio) const {
		dio << typename DataIO::my_var_uint64_t(zpath_states);
		dio << typename DataIO::my_var_uint64_t(pool  .size());
		dio << typename DataIO::my_var_uint64_t(states.size());
		dio << typename DataIO::my_var_uint64_t(firstFreeState);
		dio << typename DataIO::my_var_uint64_t(numFreeStates);
		dio << pool; // version=2
		dio.ensureWrite(&states[0], sizeof(State) * states.size());
	}

	template<class DataIO>
	friend void DataIO_loadObject(DataIO& dio, Automata& au) {
		typename DataIO::my_var_uint64_t version;
		dio >> version;
		if (version > SERIALIZATION_VERSION) {
			typedef typename DataIO::my_BadVersionException bad_ver;
			throw bad_ver(version.t, SERIALIZATION_VERSION, "Automata");
		}
		std::string state_name;
		dio >> state_name;
		if (typeid(State).name() != state_name) {
			if (1 != version.t) { // strong check for new version
				typedef typename DataIO::my_DataFormatException bad_format;
				string_appender<> msg;
				msg << "state_name[data=" << state_name
					<< " @ code=" << typeid(State).name()
					<< "]";
				throw bad_format(msg);
			}
		}
		au.load_au(dio, version.t);
	}

	template<class DataIO>
	friend void DataIO_saveObject(DataIO& dio, const Automata& au) {
		dio << typename DataIO::my_var_uint64_t(SERIALIZATION_VERSION);
		dio << std::string(typeid(State).name());
		au.save_au(dio);
	}

protected:
    MemPool pool;
    valvec<State> states;
    state_id_t    firstFreeState;
    state_id_t    numFreeStates;
    size_t transition_num;
	size_t zpath_states;

	friend size_t GraphTotalVertexes(const Automata* au) {
		return au->states.size();
	}
public:
	struct HashEq;      friend class HashEq;
	struct HashEqByMap; friend class HashEqByMap;
	struct HashEq {
        const MemPool* pool;
        HashEq(const Automata* au) : pool(&au->pool) {}
        size_t hash(const State& s) const;
        bool equal(const State& x, const State& y) const;
    };
    struct HashEqByMap {
        const unsigned char* pool;
        const state_id_t* Min;
        HashEqByMap(const Automata* au, const state_id_t* Min1)
		   	: pool(au->pool.data()), Min(Min1) {}
        size_t hash(const State& s) const;
        bool equal(const State& x, const State& y) const;
    };
};

template<class State>
size_t
Automata<State>::HashEq::hash(const State& s) const {
	assert(NULL != pool);
	assert(!s.is_pzip());
	size_t h = s.getMinch();
	if (s.is_single()) {
		h = s.get_target() * size_t(7) + h * 31;
	}
	else {
		int bits = s.rlen();
		h = h * 7 + bits;
		size_t pos = s.getpos();
		int n;
		const Transition* p;
		if (is_32bitmap(bits)) {
			uint32_t bm = pool->at<uint32_t>(pos);
			h = h * 31 + bm;
			p = &pool->at<Transition>(pos + 4);
			n = fast_popcount32(bm);
		}
		else {
			const header_max& hb = pool->at<header_max>(pos);
			bits = hb.align_bits(bits);
			for (int i = 0; i < bits/hb.BlockBits; ++i)
				h = h * 31 + hb.block(i);
			p = &pool->at<Transition>(pos + bits/8);
			n = hb.popcnt_aligned(bits);
		}
		for (int i = 0; i < n; ++i) {
			h = h * 31 + p[i];
		}
	}
	if (s.is_term())
		h = h * 3 + 1;
	return h;
}

template<class State>
bool 
Automata<State>::HashEq::equal(const State& x, const State& y) const {
	assert(NULL != pool);
	if (x.getMinch() != y.getMinch())
		return false;
	if (x.getMaxch() != y.getMaxch())
		return false;
	if (x.is_term() != y.is_term())
		return false;
	// now x.rlen() always equals to y.rlen()
	if (x.is_single())
		return x.get_target() == y.get_target();
	int bits = x.rlen();
	size_t xpos = x.getpos();
	size_t ypos = y.getpos();
	//  assert(xpos != ypos);
	int n;
	const Transition *px, *py;
	if (is_32bitmap(bits)) {
		uint32_t xbm = pool->at<uint32_t>(xpos);
		uint32_t ybm = pool->at<uint32_t>(ypos);
		if (xbm != ybm)
			return false;
		px = &pool->at<Transition>(xpos + 4);
		py = &pool->at<Transition>(ypos + 4);
		n = fast_popcount32(xbm);
	}
	else {
		const header_max& xhb = pool->at<header_max>(xpos);
		const header_max& yhb = pool->at<header_max>(ypos);
		bits = xhb.align_bits(bits);
		n = 0;
		for (int i = 0; i < bits/xhb.BlockBits; ++i) {
			if (xhb.block(i) != yhb.block(i))
				return false;
			else
				n += fast_popcount(xhb.block(i));
		}
		px = &pool->at<Transition>(xpos + bits/8);
		py = &pool->at<Transition>(ypos + bits/8);
	}
	for (int i = 0; i < n; ++i) {
		state_id_t tx = px[i];
		state_id_t ty = py[i];
		if (tx != ty)
			return false;
	}
	return true;
}

template<class State>
size_t
Automata<State>::HashEqByMap::hash(const State& s) const {
	assert(!s.is_pzip());
	size_t h = s.getMinch();
	if (s.is_single()) {
		state_id_t t = s.get_target();
		if (null_state != t)
			h = Min[t] * size_t(7) + h * 31;
	}
	else {
		assert(NULL != pool);
		int bits = s.rlen();
		h = h * 7 + bits;
		size_t pos = s.getpos();
		int n;
		const Transition* p;
		if (is_32bitmap(bits)) {
			uint32_t bm = (const uint32_t&)pool[pos];
			h = h * 31 + bm;
			p = (const Transition*)(pool + pos + 4);
			n = fast_popcount32(bm);
		}
		else {
			const header_max& hb = (const header_max&)pool[pos];
			bits = hb.align_bits(bits);
			for (int i = 0; i < bits/hb.BlockBits; ++i)
				h = h * 31 + hb.block(i);
			p = (const Transition*)(pool + pos + bits/8);
			n = hb.popcnt_aligned(bits);
		}
		for (int i = 0; i < n; ++i)
			h = h * 31 + Min[p[i]];
	}
	if (s.is_term())
		h = h * 3 + 1;
	return h;
}

template<class State>
bool 
Automata<State>::HashEqByMap::equal(const State& x, const State& y) const {
	if (x.getMinch() != y.getMinch())
		return false;
	if (x.getMaxch() != y.getMaxch())
		return false;
	if (x.is_term() != y.is_term())
		return false;
	// now x.rlen() always equals to y.rlen()
	if (x.is_single()) {
		state_id_t tx = x.get_target();
		state_id_t ty = y.get_target();
		assert(null_state == tx || null_state != Min[tx]);
		assert(null_state == ty || null_state != Min[ty]);
		if (null_state == tx)
			return null_state == ty;
		else if (null_state == ty)
			return false;
		else
			return Min[tx] == Min[ty];
	}
	assert(NULL != pool);
	int bits = x.rlen();
	size_t xpos = x.getpos();
	size_t ypos = y.getpos();
	int n;
	const Transition *px, *py;
	if (is_32bitmap(bits)) {
		uint32_t xbm = *(const uint32_t*)(pool + xpos);
		uint32_t ybm = *(const uint32_t*)(pool + ypos);
		if (xbm != ybm)
			return false;
		px = (const Transition*)(pool + xpos + 4);
		py = (const Transition*)(pool + ypos + 4);
		n = fast_popcount32(xbm);
		assert(n >= 2);
	}
	else {
		const header_max& xhb = *(const header_max*)(pool + xpos);
		const header_max& yhb = *(const header_max*)(pool + ypos);
		bits = xhb.align_bits(bits);
		n = 0;
		for (int i = 0; i < bits/xhb.BlockBits; ++i) {
			if (xhb.block(i) != yhb.block(i))
				return false;
			else
				n += fast_popcount(xhb.block(i));
		}
		px = (const Transition*)(pool + xpos + bits/8);
		py = (const Transition*)(pool + ypos + bits/8);
		assert(n >= 2);
	}
	for (int i = 0; i < n; ++i) {
		state_id_t tx = px[i];
		state_id_t ty = py[i];
		assert(null_state == tx || null_state != Min[tx]);
		assert(null_state == ty || null_state != Min[ty]);
		if (Min[tx] != Min[ty])
		   	return false;
	}
	return true;
}

template<class State>
class Automata<State>::MinADFA_onfly :
	public DFA_OnflyMinimize<Automata<State> >::UnOrdered_Parallel_Builder
{
	typedef typename
			DFA_OnflyMinimize<Automata<State> >::UnOrdered_Parallel_Builder
			super;
public:
	explicit MinADFA_onfly(Automata<State>* au, state_id_t RootState = initial_state)
	  : super(au, RootState) {}
	typedef typename
			DFA_OnflyMinimize<Automata<State> >::Ordered_Parallel_Builder
			Ordered;
};

template<class State>
class Automata<State>::MinADFA_Module_onfly :
	public	DFA_OnflyMinimize<Automata<State> >::UnOrdered_Real_Map_Builder
{
	typedef typename
			DFA_OnflyMinimize<Automata<State> >::UnOrdered_Real_Map_Builder
			super;
public:
	explicit MinADFA_Module_onfly(Automata<State>* au, state_id_t RootState = initial_state)
	  : super(au, RootState) {}
	typedef typename
			DFA_OnflyMinimize<Automata<State> >::Ordered_Real_Map_Builder
			Ordered;
};


#endif // __automata_penglei__

