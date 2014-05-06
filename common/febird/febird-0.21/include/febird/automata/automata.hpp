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

enum { Automata_MemPool_Align = 4 };

// When MyBytes is 6, intentional waste a bit: sbit is 32, nbm is 5
// When sbits==34, sizeof(PackedState)==6, StateID is uint64, the State34
#pragma pack(push,1)
//template<class StateID, int MyBytes>
template<class StateID, int sbits, int nbmBits, int Sigma = 256>
class PackedState {
	BOOST_STATIC_ASSERT(Sigma >= 256);
	BOOST_STATIC_ASSERT(Sigma <= 512);
//	enum { sbits  = MyBytes == 6 ? 32 : 8 * MyBytes - 14 };
	enum { char_bits = Sigma > 256 ? 9 : 8 };
	enum { free_flag = (1 << nbmBits) - 1  };

	StateID  spos     : sbits; // spos: saved position
//	unsigned nbm      : MyBytes == 6 ? 6 : 4; // cnt of uint32 for bitmap, in [0, 9)
	unsigned nbm      : nbmBits; // cnt of uint32 for bitmap, in [0, 9)
	unsigned term_bit : 1;
	unsigned pzip_bit : 1; // is path compressed(zipped)?
	unsigned minch    : char_bits;

	BOOST_STATIC_ASSERT((sbits + nbmBits + 2 + char_bits) % 8 == 0);

public:
	enum { sigma = Sigma };
    typedef StateID  state_id_t;
    typedef StateID  position_t;
    static const state_id_t nil_state = sbits == 32 ? UINT_MAX : state_id_t(uint64_t(1) << sbits) - 1;
    static const state_id_t max_state = nil_state; // max_state <= nil_state, and could be overridden

    PackedState() {
        spos = nil_state;
		nbm = 0;
		term_bit = 0;
		pzip_bit = 0;
        minch = 0;
    }

	void setch(auchar_t ch) {
	   	minch = ch;
		nbm = 0;
   	}
	void setMinch(auchar_t ch) {
		assert(ch < minch);
		auchar_t maxch = (0 == nbm) ? minch : getMaxch();
		nbm = (maxch - ch) / 32 + 1;
		minch = ch;
   	}
	void setMaxch(auchar_t ch) {
		assert(ch > minch);
		assert(ch > getMaxch());
		nbm = (ch - minch) / 32 + 1;
   	}
	auchar_t getMaxch() const {
	   	if (0 == nbm)
		   return minch;
	   	unsigned maxch = minch + (nbm*32 - 1);
		return std::min<int>(Sigma-1, maxch);
   	}
	auchar_t getMinch() const { return minch; }
	bool  range_covers(auchar_t ch) const { return ch == minch || (ch > minch && ch < minch + nbm*32); }
	int   rlen() const { assert(0 != nbm); return nbm * 32; }
	bool  has_children() const { return 0 != nbm || spos != nil_state; }
	bool  more_than_one_child() const { return 0 != nbm; }

    bool is_term() const { return term_bit; }
	bool is_pzip() const { return pzip_bit; }
	bool is_free() const { return free_flag == nbm; }
    void set_term_bit() { term_bit = 1; }
	void set_pzip_bit() { pzip_bit = 1; }
    void clear_term_bit() { term_bit = 0; }
	void set_free() { nbm = free_flag; }
    void setpos(size_t tpos) {
        assert(tpos % Automata_MemPool_Align == 0);
		const size_t limit = size_t(nil_state) * Automata_MemPool_Align;
        if (tpos >= limit) {
			string_appender<> msg;
			msg << BOOST_CURRENT_FUNCTION << ": too large tpos=" << tpos
				<< ", limit=" << limit;
			throw std::logic_error(msg);
		}
        spos = position_t(tpos / Automata_MemPool_Align);
    }
    size_t getpos() const { return size_t(spos) * Automata_MemPool_Align; }
    state_id_t get_target() const { return spos; }
    void set_target(state_id_t t) { spos = t; }

	typedef state_id_t transition_t;
	static transition_t first_trans(state_id_t t) { return t; }
}; // end class PackedState
#pragma pack(pop)

typedef PackedState<uint32_t, 18, 4> State4B; ///< 4 byte per-state, max_state is 256K-1
typedef PackedState<uint32_t, 26, 4> State5B; ///< 5 byte per-state, max_state is  64M-1
typedef PackedState<uint32_t, 32, 6> State6B; ///< 6 byte per-state, max_state is   4G-1
BOOST_STATIC_ASSERT(sizeof(State4B) == 4);
BOOST_STATIC_ASSERT(sizeof(State5B) == 5);
BOOST_STATIC_ASSERT(sizeof(State6B) == 6);
#if FEBIRD_WORD_BITS >= 64
typedef PackedState<uint64_t, 34, 4> State34; ///< 6 byte per-state, max_state is  16G-1
BOOST_STATIC_ASSERT(sizeof(State34) == 6);
#endif
#if FEBIRD_WORD_BITS >= 64 && defined(FEBIRD_INST_ALL_DFA_TYPES)
typedef PackedState<uint64_t, 42, 4> State7B; ///< 7 byte per-state, max_state is   2T-1
BOOST_STATIC_ASSERT(sizeof(State7B) == 7);
#endif

typedef PackedState<uint32_t, 17, 4, 448> State4B_448; ///< 4 byte per-state, max_state is 128K-1
typedef PackedState<uint32_t, 25, 4, 448> State5B_448; ///< 5 byte per-state, max_state is  32M-1
typedef PackedState<uint32_t, 32, 5, 512> State6B_512; ///< 6 byte per-state, max_state is   4G-1
BOOST_STATIC_ASSERT(sizeof(State4B_448) == 4);
BOOST_STATIC_ASSERT(sizeof(State5B_448) == 5);
BOOST_STATIC_ASSERT(sizeof(State6B_512) == 6);
#if FEBIRD_WORD_BITS >= 64 && defined(FEBIRD_INST_ALL_DFA_TYPES)
typedef PackedState<uint64_t, 41, 4, 448> State7B_448; ///< 7 byte per-state, max_state is   1T-1
BOOST_STATIC_ASSERT(sizeof(State7B_448) == 7);
#endif

/// 8 byte per-state, max_state is 4G-1
template<int Sigma>
class State32_Tmpl {
	static const int char_bits = Sigma > 256 ? 9 : 8;
    // spos: saved position
    uint32_t spos; // spos*Automata_MemPool_Align is the offset to MemPool
    unsigned minch : char_bits;  // min char
    unsigned maxch : char_bits;  // max char, inclusive
    unsigned term_bit : 1;
	unsigned pzip_bit : 1;
	unsigned free_bit : 1;

public:
	enum { sigma = Sigma };
    typedef uint32_t  state_id_t;
    typedef uint32_t  position_t;
    static const state_id_t nil_state = ~(state_id_t)0;
    static const state_id_t max_state = nil_state;

    State32_Tmpl() {
        spos = nil_state;
        minch = maxch = 0;
		term_bit = 0;
		pzip_bit = 0;
		free_bit = 0;
    }

	void setch(auchar_t ch) { minch = maxch = ch; }
	void setMinch(auchar_t ch) { minch = ch; }
	void setMaxch(auchar_t ch) { maxch = ch; }
	auchar_t getMaxch() const { return maxch; }
	auchar_t getMinch() const { return minch; }
	bool  range_covers(auchar_t ch) const { return ch >= minch && ch <= maxch; }
	int   rlen()      const { return maxch - minch + 1; }
	bool  has_children() const { return minch != maxch || spos != nil_state; }
	bool  more_than_one_child() const { return maxch != minch; }

    bool is_term() const { return term_bit; }
	bool is_pzip() const { return pzip_bit; }
	bool is_free() const { return free_bit; }
    void set_term_bit() { term_bit = 1; }
	void set_pzip_bit() { pzip_bit = 1; }
    void clear_term_bit() { term_bit = 0; }
	void set_free() { free_bit = 1; }
    void setpos(size_t tpos) {
        if (tpos >= size_t(nil_state) * Automata_MemPool_Align) {
			THROW_STD(logic_error
				, "too large tpos=%zd, nil_state=%zd"
				, tpos, (size_t)nil_state);
		}
        this->spos = position_t(tpos / Automata_MemPool_Align);
    }
    size_t getpos() const {
        return size_t(spos) * Automata_MemPool_Align;
    }
    state_id_t get_target() const { return spos; }
    void set_target(state_id_t t) { spos = t;    }

	typedef state_id_t transition_t;
	static transition_t first_trans(state_id_t t) { return t; }
}; // end class State32_Tmpl

typedef State32_Tmpl<256> State32;
typedef State32_Tmpl<512> State32_512;

/// State must has trivial destructor
template<class State>
class Automata : public DFA_Interface, public DFA_MutationInterface
{
	typedef typename State::transition_t Transition;
    typedef MemPool<Automata_MemPool_Align> MyMemPool;
public:
	class MinADFA_Module_onfly;
	class MinADFA_onfly;
	typedef State      state_t;
	typedef typename State::transition_t transition_t;
    typedef typename State::position_t position_t;
    typedef typename State::state_id_t state_id_t;
    static const int edge_size = sizeof(Transition);
	enum { sigma = State::sigma };
    static const state_id_t max_state = State::max_state;
    static const state_id_t nil_state = State::nil_state;

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
		m_dyn_sigma = sigma;
        states.push_back(State()); // initial_state
        firstFreeState = nil_state;
        numFreeStates  = 0;
        transition_num = 0;
		zpath_states = 0;
	}

public:

    Automata() : pool(sizeof(Transition)*sigma + sigma/8) { init(); }
	void clear() { states.clear(); pool.clear(); init(); }
	void erase_all() { states.erase_all(); pool.erase_all(); init(); }

	const valvec<State>& internal_get_states() const { return states; }
	const MyMemPool    & internal_get_pool  () const { return pool  ; }
    size_t total_states() const { return states.size(); }
    size_t total_transitions() const { return transition_num; }
    size_t new_state() override final {
        state_id_t s;
        if (nil_state == firstFreeState) {
            assert(numFreeStates == 0);
            s = states.size();
            if (s >= State::max_state) {
                THROW_STD(runtime_error, "exceeding max_state=%zd"
						, (size_t)State::max_state);
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
        if (s.more_than_one_child()) {
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
			memcpy(&pool.at<byte_t>(pos), &pool.at<byte_t>(s.getpos()), nb);
        }
		else if (s.is_pzip()) {
			// very unlikely:
			assert(0); // now, unreachable
			int nzc = pool.at<byte_t>(sizeof(state_id_t) + s.getpos());
			size_t nb = sizeof(state_id_t) + nzc + 1;
			size_t pos = pool.alloc(pool.align_to(nb));
			memcpy(&pool.at<byte_t>(pos), &pool.at<byte_t>(s.getpos()), nb);
			states[y].setpos(pos);
		}
		else if (nil_state != s.get_target()) {
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
		if (x.more_than_one_child()) { // has transitions in pool
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
	   	else if (x.get_target() != nil_state)
			transition_num -= 1; // just 1 transition
		// put s to free list, states[s] is not a 'State' from now
		x.set_target(firstFreeState);
		x.set_free();
		firstFreeState = s;
		numFreeStates++;
    }
	void del_all_move(size_t s) {
		assert(0 == zpath_states);
        assert(s < states.size());
		State& x = states[s];
		assert(!x.is_pzip()); // must be non-path-zipped
		if (x.more_than_one_child()) { // has transitions in pool
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
	   	else if (x.get_target() != nil_state)
			transition_num -= 1; // just 1 transition
		x.set_target(nil_state);
		x.setch(0);
	}
	state_id_t min_free_state() const {
		state_id_t s = firstFreeState;
		state_id_t s_min = states.size()-1;
		while (nil_state != s) {
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
	bool more_than_one_child(state_id_t s) const {
        assert(s < (state_id_t)states.size());
		return states[s].more_than_one_child();
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
		assert(!states[parent].more_than_one_child());
		return states[parent].get_target();
	}
	auchar_t get_single_child_char(state_id_t parent) const {
		assert(0 == zpath_states);
        assert(parent < (state_id_t)states.size());
		assert(!states[parent].more_than_one_child());
		return states[parent].getMinch();
	}

    Transition state_move(state_id_t curr, auchar_t ch) const {
        assert(curr < (state_id_t)states.size());
		ASSERT_isNotFree(curr);
        return state_move(states[curr], ch);
	}
    Transition state_move(const State& s, auchar_t ch) const {
        assert(s.getMinch() <= s.getMaxch());
        if (!s.range_covers(ch))
            return nil_state;
        if (!s.more_than_one_child())
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
        return nil_state;
    }

	using DFA_MutationInterface::add_all_move;
	void add_all_move(size_t s, const CharTarget<size_t>* trans, size_t n)
	override final {
		assert(s < states.size());
		State& x = states[s];
		assert(!x.is_free());
		assert(!x.is_pzip());
		assert(x.get_target() == nil_state);
		assert(n <= sigma);
		transition_num += n;
		if (0 == n) return; // do nothing
		if (1 == n) {
			x.set_target(trans[0].target);
			x.setch(trans[0].ch);
			return;
		}
	#ifndef NDEBUG
		for (size_t i = 1; i < n; ++i)
			assert(trans[i-1].ch < trans[i].ch);
	#endif
		auchar_t minch = trans[0].ch;
		size_t bits = bm_real_bits(trans[n-1].ch - minch + 1);
		assert(bits % 32 == 0);
		size_t size = pool.align_to(bits/8 + sizeof(transition_t)*n);
		size_t pos = pool.alloc(size);
		if (is_32bitmap(bits)) {
			uint32_t& bm = pool.at<uint32_t>(pos);
			bm = 0;
			for (size_t i = 0; i < n; ++i) {
				auchar_t ch = trans[i].ch;
				bm |= uint32_t(1) << (ch-minch);
			}
		} else {
			header_max& bm = pool.at<header_max>(pos);
			assert((char*)bm.base(bits) == (char*)&bm + bits/8);
			memset(&bm, 0, bits/8);
			for (size_t i = 0; i < n; ++i) {
				auchar_t ch = trans[i].ch;
				bm.set1(ch-minch);
			}
		}
		transition_t* t = &pool.at<transition_t>(pos+bits/8);
		for (size_t i = 0; i < n; ++i) {
			new (t+i) transition_t (trans[i].target);
		}
		x.setpos(pos);
		x.setch(minch); // setMinch has false positive assert()
		x.setMaxch(trans[n-1].ch);
	}

protected:
    size_t 
	add_move_imp(size_t source, size_t target, auchar_t ch, bool OverwriteExisted)
	override final {
        assert(source < states.size());
        assert(target < states.size());
		ASSERT_isNotFree(source);
		ASSERT_isNotFree(target);
		assert(!states[source].is_pzip());
	//	assert(!states[target].is_pzip()); // target could be path zipped
        State& s = states[source];
        assert(s.getMinch() <= s.getMaxch());
        if (!s.more_than_one_child()) {
			const size_t old_target = s.get_target();
            if (nil_state == old_target) {
            //	assert(0 == s.getMinch()); // temporary assert
                s.setch(ch);
                s.set_target(target);
				transition_num++;
				return nil_state;
            }
            if (s.getMinch() == ch) {
                // the target for ch has existed
				if (OverwriteExisted)
					s.set_target(target);
                return old_target;
            }
			auchar_t minch = s.getMinch();
			auchar_t maxch = minch;
		//	assert(0 != minch); // temporary assert
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
            return nil_state;
        }
        assert(nil_state != s.get_target());
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
		return nil_state;
    }
public:
	size_t trans_num(state_id_t curr) const {
        const State s = states[curr];
        if (!s.more_than_one_child()) {
            state_id_t target = single_target(s);
			return (nil_state == target) ? 0 : 1;
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
        if (!s.more_than_one_child()) {
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
		// default single_target is nil_state, just the designated
		return s.is_pzip() ? pool.at<state_id_t>(s.getpos()) : s.get_target();
	}

    // just walk the (curr, dest), don't access the 'ch'
    // this is useful such as when minimizing DFA
    template<class OP> // use ctz (count trailing zero)
    void for_each_dest(state_id_t curr, OP op) const {
        const State& s = states[curr];
        if (!s.more_than_one_child()) {
            state_id_t target = single_target(s);
            if (nil_state != target)
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
        if (!s.more_than_one_child()) {
            state_id_t target = single_target(s);
            if (nil_state != target)
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
        if (!s.more_than_one_child()) {
			state_id_t target = single_target(s);
			if (nil_state != target)
				op(curr, State::first_trans(target), s.getMinch());
			return;
		}
        const int bits = s.rlen();
        if (is_32bitmap(bits)) {
            const header_b32& hb = pool.at<header_b32>(s.getpos());
            uint32_t bm = hb.b;
            auchar_t ch = s.getMinch();
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
                auchar_t ch = s.getMinch() + i * hb.BlockBits;
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
        if (!s.more_than_one_child()) {
			state_id_t target = single_target(s);
			if (nil_state != target)
				op(curr, State::first_trans(target), s.getMinch());
			return;
		}
        const int bits = s.rlen();
        if (is_32bitmap(bits)) {
            const header_b32& hb = pool.at<header_b32>(s.getpos());
            uint32_t bm = hb.b;
            auchar_t ch = s.getMinch();
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
                auchar_t ch = s.getMinch() + i * hb.BlockBits;
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
        if (!s.more_than_one_child()) {
            state_id_t target = single_target(s);
            if (nil_state != target)
                op(curr, State::first_trans(target), s.getMinch());
			return;
        }
        const int bits = s.rlen();
        if (is_32bitmap(bits)) {
            const header_b32& hb = pool.at<header_b32>(s.getpos());
            uint32_t bm = hb.b;
            auchar_t ch = s.getMinch();
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
                auchar_t ch = s.getMinch() + i * hb.BlockBits;
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
	
    void resize_states(size_t new_states_num) {
		if (new_states_num >= max_state) {
			THROW_STD(logic_error
				, "new_states_num=%zd, exceed max_state=%zd"
				, new_states_num, (size_t)max_state);
		}
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

	void compact() {}

    void swap(Automata& y) {
		assert(typeid(*this) == typeid(y));
		DFA_Interface::swap(y);
        pool  .swap(y.pool  );
        states.swap(y.states);
		std::swap(firstFreeState, y.firstFreeState);
		std::swap(numFreeStates , y.numFreeStates );
        std::swap(transition_num, y.transition_num);
        std::swap(zpath_states  , y.zpath_states  );
    }

	void path_zip_freeze(size_t n_zpath_states) {
   		pool.shrink_to_fit();
		zpath_states = n_zpath_states;
	}

    void add_zpath(state_id_t ss, const byte_t* str, size_t len) {
        assert(len <= 255);
		assert(!states[ss].is_pzip());
		byte_t* p;
		size_t  newpos;
		if (!states[ss].more_than_one_child()) {
			newpos = pool.alloc(pool.align_to(sizeof(state_id_t) + len+1));
			pool.at<state_id_t>(newpos) = states[ss].get_target();
			p = &pool.at<byte_t>(newpos + sizeof(state_id_t));
		}
		else {
			size_t oldpos = states[ss].getpos();
			size_t oldlen = trans_bytes(ss);
			size_t newlen = pool.align_to(oldlen + len + 1);
			newpos = pool.alloc3(oldpos, pool.align_to(oldlen), newlen);
			p = &pool.at<byte_t>(newpos + oldlen);
		}
		states[ss].setpos(newpos);
		p[0] = len;
		memcpy(p+1, str, len);
		states[ss].set_pzip_bit();
    }

	size_t get_zipped_path_str_pos(const State& s) const {
		assert(s.is_pzip());
		size_t xpos;
		if (!s.more_than_one_child())
			// before xpos, it is the single_target
			xpos = s.getpos() + sizeof(state_id_t);
		else
			xpos = s.getpos() + trans_bytes(s);
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

	enum { SERIALIZATION_VERSION = 4 };

	template<class DataIO> void load_au(DataIO& dio, size_t version) {
		typename DataIO::my_var_uint64_t n_pool, n_states;
		typename DataIO::my_var_uint64_t n_firstFreeState, n_numFreeStates;
		typename DataIO::my_var_uint64_t n_zpath_states;
		dio >> n_zpath_states;
		dio >> n_pool; // keeped but not used in version=2
		dio >> n_states;
		dio >> n_firstFreeState;
		dio >> n_numFreeStates;
		if (version >= 4)
			this->load_kv_delim_and_is_dag(dio);
		zpath_states = n_zpath_states;
		states.resize_no_init(n_states.t);
		if (1 == version) {
			pool.resize_no_init(n_pool.t);
			if (pool.size())
				dio.ensureRead(&pool.template at<byte_t>(0), n_pool.t);
		}
		else {
			if (version > SERIALIZATION_VERSION) {
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
		if (version < 4) {
			this->m_is_dag = this->tpl_is_dag();
			// m_kv_delim is absent in version < 4
		}
	}
	template<class DataIO> void save_au(DataIO& dio) const {
		dio << typename DataIO::my_var_uint64_t(zpath_states);
		dio << typename DataIO::my_var_uint64_t(pool  .size());
		dio << typename DataIO::my_var_uint64_t(states.size());
		dio << typename DataIO::my_var_uint64_t(firstFreeState);
		dio << typename DataIO::my_var_uint64_t(numFreeStates);
		dio << uint16_t(this->m_kv_delim); // version >= 4
		dio << char(this->m_is_dag); // version >= 4
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
#if 0 // don't check
	  // but keep load state_name for data format compatible
	  // by using DFA_Interface::load_from, check is unneeded
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
#endif
		au.load_au(dio, version.t);
	}

	template<class DataIO>
	friend void DataIO_saveObject(DataIO& dio, const Automata& au) {
		dio << typename DataIO::my_var_uint64_t(SERIALIZATION_VERSION);
		dio << fstring(typeid(State).name());
		au.save_au(dio);
	}

	// implement in dfa_load_save.cpp
	void finish_load_mmap(const DFA_MmapHeader*);
	void prepare_save_mmap(DFA_MmapHeader*, const void**) const;
//	size_t mempool_meta_size() const;

	~Automata() {
		if (this->mmap_base) {
			pool.get_data_byte_vec().risk_release_ownership();
			states.risk_release_ownership();
		}
	}

protected:
	MyMemPool pool;
    valvec<State> states;
    state_id_t    firstFreeState;
    state_id_t    numFreeStates;
    size_t transition_num;
	using DFA_Interface::zpath_states;

	friend size_t
	GraphTotalVertexes(const Automata* au) { return au->states.size(); }
public:
	struct HashEq;      friend class HashEq;
	struct HashEq {
        const MyMemPool* pool;
        HashEq(const Automata* au) : pool(&au->pool) {}
        size_t hash(const State& s) const;
        bool equal(const State& x, const State& y) const;
    };
	size_t adfa_hash_hash(const state_id_t* Min, size_t state_id) const;
	bool adfa_hash_equal(const state_id_t* Min, size_t x_id, size_t y_id) const;
typedef Automata MyType;
#include "ppi/dfa_match.hpp"
#include "ppi/dfa_reverse.hpp"
#include "ppi/dfa_hopcroft.hpp"
#include "ppi/dfa_const_virtuals.hpp"
#include "ppi/post_order.hpp"
#include "ppi/adfa_minimize.hpp"
#include "ppi/path_zip.hpp"
#include "ppi/dfa_mutation_virtuals.hpp"
#include "ppi/dfa_methods_calling_swap.hpp"
};

template<class State>
size_t
Automata<State>::HashEq::hash(const State& s) const {
	assert(NULL != pool);
	assert(!s.is_pzip());
	size_t h = s.getMinch();
	if (!s.more_than_one_child()) {
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
		h = h * 5 + n;
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
	if (!x.more_than_one_child())
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
Automata<State>::adfa_hash_hash(const state_id_t* Min, size_t state_id) const {
	const State& s = states[state_id];
	assert(!s.is_pzip());
	size_t h = s.getMinch();
	if (!s.more_than_one_child()) {
		state_id_t t = s.get_target();
		if (nil_state != t)
			h = Min[t] * size_t(7) + h * 31;
	}
	else {
		const byte_t* pool = this->pool.data();
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
		h = h * 5 + n;
	}
	if (s.is_term())
		h = h * 3 + 1;
	return h;
}

template<class State>
bool 
Automata<State>::adfa_hash_equal(const state_id_t* Min, size_t x_id, size_t y_id)
const {
	const State& x = states[x_id];
	const State& y = states[y_id];
	if (x.getMinch() != y.getMinch())
		return false;
	if (x.getMaxch() != y.getMaxch())
		return false;
	if (x.is_term() != y.is_term())
		return false;
	// now x.rlen() always equals to y.rlen()
	if (!x.more_than_one_child()) {
		state_id_t tx = x.get_target();
		state_id_t ty = y.get_target();
		assert(nil_state == tx || nil_state != Min[tx]);
		assert(nil_state == ty || nil_state != Min[ty]);
		if (nil_state == tx)
			return nil_state == ty;
		else if (nil_state == ty)
			return false;
		else
			return Min[tx] == Min[ty];
	}
	const byte_t* pool = this->pool.data();
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
		assert(nil_state == tx || nil_state != Min[tx]);
		assert(nil_state == ty || nil_state != Min[ty]);
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

