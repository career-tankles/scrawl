#ifndef __automata_penglei__
#define __automata_penglei__

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

typedef unsigned char  char_t;
typedef unsigned char  byte_t;

template<class T, class BaseCont>
const T&
QueueOrStack_get(const std::queue<T, BaseCont>& q) { return q.front(); }
template<class T, class BaseCont>
const T&
QueueOrStack_get(const std::stack<T, BaseCont>& s) { return s.top(); }

template<class QueueOrStack>
struct QueueOrStack_is_stack;
template<class T, class BaseCont>
struct QueueOrStack_is_stack<std::queue<T, BaseCont> > : boost::false_type {};
template<class T, class BaseCont>
struct QueueOrStack_is_stack<std::stack<T, BaseCont> > : boost:: true_type {};

#ifdef TRACE_AUTOMATA
    #define AU_TRACE printf
#else
//  #define AU_TRACE 1 ? 0 : printf
    #define AU_TRACE(format, ...)
#endif

template<class StateID>
struct Hopcroft {
	typedef StateID BlockID; // also served as blid block_id
	struct Elem {
		StateID  next;
		StateID  prev;
		BlockID  blid;
	};
	struct Block {
		StateID  head;
//		StateID  blen; // block length, use integer type StateID
		StateID  blen : sizeof(StateID)*8-1;
		unsigned isInW: 1;
		Block() : head(-1), blen(0), isInW(0) {}
	};
	const static StateID max_blen = StateID(StateID(1) << (sizeof(StateID)*8-1)) - 1;
    struct Splitter : valvec<StateID> {
        explicit Splitter() {
            this->ch = -1;
            this->reserve(32);
        }
        void sort() {
            assert(-1 != ch);
            std::sort(this->begin(), this->end());
        }
        void resize0() {
            this->ch = -1;
            this->resize(0);
            if (this->capacity() > 8*1024)
                this->shrink_to_fit();
        }
        void insert(StateID x) { this->push_back(x); }
        int ch;
    };
	valvec<Elem>  L; // parallel with Automata::states
	valvec<Block> P; // partition

	template<class State>
	explicit Hopcroft(const valvec<State>& states) {
		L.resize_no_init(states.size());
		P.resize(2);
		for (size_t s = 0; s < states.size(); ++s) {
			BlockID blid = states[s].is_term() ? 1 : 0;
			if (P[blid].blen == max_blen)
				throw std::length_error(BOOST_CURRENT_FUNCTION);
			add(blid, s);
		}
		if (0 == P[0].blen) std::swap(P[0], P[1]);
		if (0 == P[1].blen) P.pop_back();
	}

	// add 's' to end of P[g], P[g] is a double linked cyclic list
	void add(BlockID g, StateID s) {
		if (P[g].blen == 0) {
			P[g].head = s;
			L[s].next = s;
			L[s].prev = s;
		} else {
			StateID h = P[g].head;
			StateID t = L[h].prev;
        //  assert(h <= t);
		//	assert(s > t);
			L[s].next = h;
			L[s].prev = t;
			L[h].prev = s;
			L[t].next = s;
		}
		P[g].blen++;
		L[s].blid = g;
	}
	void del(BlockID g, StateID s) {
		assert(L[s].blid == g);
		assert(P[g].blen >= 1);
    //  assert(P[g].head <= s);
		if (P[g].head == s)
			P[g].head = L[s].next;
		P[g].blen--;
		L[L[s].prev].next = L[s].next;
		L[L[s].next].prev = L[s].prev;
	}
    void printSplitter(const char* sub, const Splitter& z) {
#ifdef TRACE_AUTOMATA
        printf("%s[%c]: ", sub, z.ch);
        for (size_t i = 0; i < z.size(); ++i) {
            printf("(%ld %ld)", (long)z[i], (long)L[z[i]].blid);
        }
        printf("\n");
#endif
    }
    void printMappings() {
#ifdef TRACE_AUTOMATA
        for (BlockID b = 0; b < P.size(); ++b) {
            const Block B = P[b];
            StateID s = B.head;
            printf("Block:%02ld: ", (long)b);
            do {
                printf("%ld ", (long)s);
                s = L[s].next;
            } while (B.head != s);
            printf("\n");
        }
#endif
    }
	void printBlock(BlockID blid) const {
#ifdef TRACE_AUTOMATA
		const BlockID head = P[blid].head;
		StateID s = head;
		printf("    block[%ld]:", (long)blid);
		do {
			printf(" %ld", (long)s);
			s = L[s].next;
		} while (head != s);
		printf("\n");
#endif
	}

	BlockID minBlock(BlockID b1, BlockID b2) const {
		if (P[b1].blen < P[b2].blen)
			return b1;
		else
			return b2;
	}

	template<class E_Gen> void refine(E_Gen egen) {
		assert(P.size() >= 1);
#ifdef WAITING_SET_FIFO
		struct WaitingSet : std::deque<BlockID> {};
#else
		struct WaitingSet : valvec<BlockID> {};
#endif
		WaitingSet   W;
#if 0
		auto putW = [&](BlockID b) {
			W.push_back(b);
			P[b].isInW = 1;
		};
		auto takeW = [&]() -> BlockID {
	#ifdef WAITING_SET_FIFO
			BlockID b = W.front();
			W.pop_front();
	#else
			BlockID b = W.back();
			W.pop_back();
	#endif
			P[b].isInW = 0;
			return b;
		};
#else
	#define putW(b) ({BlockID b2 = b; W.push_back(b2); P[b2].isInW = 1;})
	#define takeW() ({BlockID b = W.back(); W.pop_back(); P[b].isInW = 0; b;})
#endif
	smallmap<Splitter> C_1(256); // set of p which move(p,ch) is in Block C
	valvec<BlockID> bls; // candidate splitable Blocks
	{// Init WaitingSet 
		//putW(P.size()==1 ? 0 : minBlock(0,1));
		for (int b = 0; b < (int)P.size(); ++b)
			putW(b);
	}
	long iterations = 0;
	long num_split = 0;
	long non_split_fast = 0;
	long non_split_slow = 0;
	while (!W.empty()) {
		const BlockID C = takeW();
		{
			C_1.resize0();
			StateID head = P[C].head;
			StateID curr = head;
			do {
				egen(curr, C_1);
				curr = L[curr].next;
			} while (curr != head);
		}
AU_TRACE("iteration=%03ld, size[P=%ld, W=%ld, C_1=%ld], C=%ld\n"
        , iterations, (long)P.size(), (long)W.size(), (long)C_1.size()
        , (long)C);
//-split----------------------------------------------------------------------
for (size_t j = 0; j < C_1.size(); ++j) {
	Splitter& E = C_1.byidx(j);
	assert(!E.empty());
//  E.sort();
    size_t Esize = E.size();
{//----Generate candidate splitable set: bls---------------
	bls.reserve(Esize);
	bls.resize(0);
	size_t iE1 = 0, iE2 = 0;
	for (; iE1 < Esize; ++iE1) {
		StateID sE = E[iE1];
		BlockID blid = L[sE].blid;
		if (P[blid].blen != 1) { // remove State which blen==1 in E
			E[iE2++] = sE;
			bls.unchecked_push_back(blid);
		}
		assert(P[blid].blen > 0);
	}
	E.resize(Esize = iE2);
	bls.trim(std::unique(bls.begin(), bls.end()));
	if (bls.size() == 1) {
		BlockID blid = bls[0];
		if (E.size() == P[blid].blen) {
	//		printf("prefilter: E(s=%ld,c=%c) is not a splitter\n", (long)blid, E.ch);
			non_split_fast++;
			continue; // not a splitter
		}
		assert(E.size() < P[blid].blen);
	//	printf("single splitter\n");
	} else {
		std::sort(bls.begin(), bls.end());
		bls.trim(std::unique(bls.begin(), bls.end()));
	}
//	if (bls.size() > 1) printf("bls.size=%ld\n", (long)bls.size());
}//----End Generate candidate splitable set: bls---------------

#if defined(TRACE_AUTOMATA)
	if (bls.size() > 1)
	    printSplitter("  SPLITTER", E);
    size_t Bsum = 0; for (auto x : bls) Bsum += P[x].blen;
    if (bls.size() && Esize > 2*Bsum)
        printf("bigE=%ld, Bsum=%ld\n", (long)Esize, (long)Bsum);
#endif
    for (size_t i = 0; i < bls.size(); ++i) {
        assert(Esize > 0);
        const BlockID B_E = bls[i];   // (B -  E) take old BlockID of B
        const BlockID BxE = P.size(); // (B /\ E) take new BlockID
        const Block B = P[B_E]; // copy, not ref
        assert(B.blen > 1);
        printSplitter("  splitter", E);
//      printBlock(B_E);
        P.push_back();  // (B /\ E) will be in P.back() -- BxE
        size_t iE1 = 0, iE2 = 0;
        for (; iE1 < Esize; ++iE1) {
            StateID sE = E[iE1];
            if (L[sE].blid == B_E) {
                del(B_E, sE); // (B -  E) is in B_E
                add(BxE, sE); // (B /\ E) is in BxE
            }
		   	else E[iE2++] = sE; // E - B
        }
        E.resize(Esize = iE2); // let E = (E - B)
        assert(P[BxE].blen); // (B /\ E) is never empty
        assert(P[B_E].blen < B.blen); // assert P[B_E] not roll down
        if (P[B_E].blen == 0) { // (B - E) is empty
            P[B_E] = B; // restore the block
            fix_blid(B_E);
            P.pop_back(); // E is not a splitter for B
            non_split_slow++;
        } else {
			num_split++;
			if (P[B_E].isInW)
				putW(BxE);
			else
				putW(minBlock(B_E, BxE));
		}
    } // for
}
//-end split------------------------------------------------------------------
        ++iterations;
    }
#if defined(TRACE_AUTOMATA)
	printMappings();
	printf("refine completed, iterations=%ld, num_split=%ld, non_split[fast=%ld, slow=%ld], size[org=%ld, min=%ld]\n"
		  , iterations, num_split, non_split_fast, non_split_slow, (long)L.size(), (long)P.size());
#endif
        if (P[0].head != 0) { // initial state is not in block 0
            // put initial state into block 0, 0 is the initial state
            BlockID initial_block = L[0].blid;
            std::swap(P[0], P[initial_block]);
            fix_blid(0);
            fix_blid(initial_block);
        }
	} // refine1

	void fix_blid(size_t g) {
        const StateID B_head = P[g].head;
        StateID s = B_head;
        do {
            L[s].blid = g;
            s = L[s].next;
        } while (s != B_head);
    }
};

#ifdef  ENABLE_DUP_PATH_POOL
	#define IF_DUP_PATH_POOL(stmt) stmt
	#define ASSERT_isNotFree(s)       assert(!this->is_free(s))
	#define ASSERT_isNotFree2(au,s)   assert(!au->is_free(s))
#else
	#define IF_DUP_PATH_POOL(stmt)
	#define ASSERT_isNotFree(s)       (void)0
	#define ASSERT_isNotFree2(au,s)   (void)0
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
    void set_term_bit() { term_bit = 1; }
	void set_pzip_bit() { pzip_bit = 1; }
#ifdef  ENABLE_DUP_PATH_POOL
	bool is_pool() const { return pool_bit; }
	void set_pool_bit() { pool_bit = 1; }
#endif
    void setpos(size_t tpos) {
        assert(tpos % MemPool::align_size == 0);
        if (tpos >= size_t(null_state) * MemPool::align_size) {
			throw std::logic_error(std::string(BOOST_CURRENT_FUNCTION) + ": too large tpos");
		}
        spos = position_t(tpos / MemPool::align_size);
    }
    size_t getpos() const { return size_t(spos) * MemPool::align_size; }
    state_id_t get_target() const { return spos; }
    void set_target(state_id_t t) { spos = t; }
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
    void set_term_bit() { term_bit = 1; }
	void set_pzip_bit() { pzip_bit = 1; }
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
}; // end class State32

inline uint32_t // uint32_t is as state_id_t
first_trans(uint32_t target, uint32_t* /*for dispatch*/) { return target; }
inline uint64_t // uint64_t is as state_id_t
first_trans(uint64_t target, uint64_t* /*for dispatch*/) { return target; }

inline unsigned short target_of(unsigned short t) { return t; }
inline unsigned       target_of(unsigned       t) { return t; }
inline unsigned long  target_of(unsigned long  t) { return t; }
inline unsigned long long target_of(unsigned long long t) { return t; }

/// Transition must be constructed from state_id_t
/// Transition may have additional attributes other than state_id_t
/// State must has trivial destructor
template<class State, class Transition = typename State::state_id_t>
class Automata {
public:
	class MinADFA_onfly;  friend class MinADFA_onfly; // must #include "dawg.hpp"
	typedef State      state_t;
	typedef Transition transition_t;
    typedef typename State::position_t position_t;
    typedef typename State::state_id_t state_id_t;
	typedef valvec<std::map<char_t, valvec<state_id_t> > > nfa_t;
    static const  int edge_size = sizeof(Transition);
    static const  int char_bits = 8;
    static const  int sigma = 1 << char_bits; // normally is 256
    static const  state_id_t  max_state = State:: max_state;
    static const  state_id_t null_state = State::null_state;
//  static inline state_id_t null_state() { return State::null_state; }

	struct CharTarget {
		short       c;
		state_id_t  t;
		CharTarget(short c, state_id_t t) : c(c), t(t) {}
	};
	struct CompactReverseNFA {
		void clear() {
			index.clear();
			data.clear();
		}
		valvec<state_id_t> index; // parallel with Automata::states
		valvec<CharTarget> data;
	};

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
#if !defined(NDEBUG)
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
        //  assert(target_of(p[i]) >= 0); // state_id_t is unsigned
			ASSERT_isNotFree(target_of(p[i]));
			assert(target_of(p[i]) < (state_id_t)states.size());
        }
#endif
    }

	void init() {
        states.push_back(State()); // initial_state
        firstFreeState = null_state;
        numFreeStates  = 0;
        transition_num = 0;
		zpath_states = false;
		IF_DUP_PATH_POOL(allow_zpath_pool = false;)
	}

public:
    Automata() : pool(sizeof(Transition)*sigma + sigma/8) { init(); }
	void clear() { states.clear(); pool.clear(); init(); }

	const valvec<State>& internal_get_states() const { return states; }
	const MemPool      & internal_get_pool  () const { return pool  ; }
    size_t total_states() const { return states.size(); }
    size_t total_transitions() const { return transition_num; }
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
    state_id_t clone_state(state_id_t source) {
        assert(source < states.size());
        state_id_t y = new_state();
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
    void del_state(state_id_t s) {
		assert(!zpath_states);
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
IF_DUP_PATH_POOL(x.set_pool_bit();) // is_free == (!is_pzip() && is_pool())
		// put s to free list, states[s] is not a 'State' from now
		x.set_target(firstFreeState);
		firstFreeState = s;
		numFreeStates++;
    }
	size_t num_free_states() const { return numFreeStates; }
	size_t num_used_states() const { return states.size() - numFreeStates; }

#ifdef  ENABLE_DUP_PATH_POOL
	bool is_free(const State& s) const { return !s.is_pzip() && s.is_pool(); }
    bool is_free(state_id_t s) const {
        assert(size_t(s) < states.size());
        return is_free(states[s]);
    }
#endif
    bool is_term(state_id_t s) const {
        assert(s < (state_id_t)states.size());
        return states[s].is_term();
    }
    void set_term_bit(state_id_t s) {
        assert(s < (state_id_t)states.size());
        states[s].set_term_bit();
    }

	bool accept(const std::string& s)const{return accept(&s[0],s.size()); }
	bool accept(const fstring s) const { return accept(s.p, s.n); }
	bool accept(const char* str) const { return accept(str, strlen(str)); }
	bool accept(const char* str, size_t len) const {
		state_id_t curr = initial_state;
		for (size_t i = 0; ; ++i) {
			const State& s = states[curr];
			if (s.is_pzip()) {
				size_t zpos = get_zipped_path_str_pos(s);
				const byte_t* s2 = &pool.at<byte_t>(zpos);
				size_t n = *s2++;
				assert(n > 0);
				if (i + n > len)
					return false;
				size_t j = 0;
				do { // prefer do .. while for performance
					if ((byte_t)str[i++] != s2[j++])
						return false;
				} while (j < n);
			}
			assert(i <= len);
			if (i == len)
				return s.is_term();
			state_id_t next = target_of(state_move(s, str[i]));
			if (null_state == next)
				return false;
			assert(next < states.size());
			ASSERT_isNotFree(next);
			curr = next;
		}
	}

	/// @param on_match(const char* str, size_t endpos)
	/// @return max forward scaned chars
	template<class OnMatch>
	size_t match_prefix(const char* str, size_t len, OnMatch on_match) const {
		state_id_t curr = initial_state;
		for (size_t i = 0; ; ++i) {
			const State& s = states[curr];
			if (s.is_pzip()) {
				size_t zpos = get_zipped_path_str_pos(s);
				const byte_t* s2 = &pool.at<byte_t>(zpos);
				size_t n = std::min<size_t>(*s2++, len-i);
				for (size_t j = 0; j < n; ++i, ++j)
					if ((byte_t)str[i] != s2[j]) return i;
			}
			assert(i <= len);
			if (s.is_term())
				on_match(str, i);
			if (i == len)
				return len;
			state_id_t next = target_of(state_move(s, str[i]));
			if (null_state == next)
				return i;
			assert(next < states.size());
			ASSERT_isNotFree(next);
			curr = next;
		}
		assert(0);
	}

	/// @return pos <  len     : the mismatch pos
	///         pos == len     : matched all
	///         pos == len + 1 : matched state is not a final state
	size_t first_mismatch_pos(const char* str, size_t len) const {
		state_id_t curr = initial_state;
		for (size_t i = 0; i < len; ++i) {
			const State s = states[curr];
			if (s.is_pzip()) {
				size_t zpos = get_zipped_path_str_pos(s);
				const byte_t* s2 = &pool.at<byte_t>(zpos);
				size_t n = *s2++;
				assert(n > 0);
				size_t m = i + n > len ? len - i : n;
				size_t j = 0;
				do { // prefer do .. while for performance
					if ((byte_t)str[i] != s2[j])
						return i;
					++i; ++j;
				} while (j < m);
				if (i == len)
					return s.is_term() ? len : len + 1;
			}
			state_id_t next = target_of(state_move(s, str[i]));
			if (null_state == next)
				return i;
			assert(next < states.size());
			ASSERT_isNotFree(next);
			curr = next;
		}
		const State& s = this->states[curr];
		return !s.is_pzip() && s.is_term() ? len : len + 1;
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
			return first_trans(single_target(s), (Transition*)NULL);
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
    void add_move_checked(const state_id_t source, const state_id_t target, const char_t ch) {
		state_id_t  old = add_move_imp(source, target, ch, false);
		assert(null_state == old);
		if (null_state != old) {
			throw std::logic_error(BOOST_CURRENT_FUNCTION);
		}
	}
    state_id_t add_move(const state_id_t source, const state_id_t target, const char_t ch) {
		return add_move_imp(source, target, ch, false);
	}
	// overwrite existed
	/// @return null_state : if not existed
	///         otherwise  : the old target which was overwritten
    state_id_t set_move(const state_id_t source, const state_id_t target, const char_t ch) {
		return add_move_imp(source, target, ch, true);
	}
protected:
    state_id_t add_move_imp(const state_id_t source, const state_id_t target
			, const char_t ch, bool OverwriteExisted) {
        assert(source < (state_id_t)states.size());
        assert(target < (state_id_t)states.size());
		ASSERT_isNotFree(source);
		ASSERT_isNotFree(target);
		assert(!states[source].is_pzip());
	//	assert(!states[target].is_pzip()); // target could be path zipped
        State& s = states[source];
        assert(s.getMinch() <= s.getMaxch());
        if (s.is_single()) {
			const state_id_t old_target = s.get_target();
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
                trans[1] = first_trans(old_target, (Transition*)NULL);
            } else {
                trans[0] = first_trans(old_target, (Transition*)NULL);
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
					state_id_t old = target_of(hb.s[inspos]);
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
					state_id_t old = target_of(hb.base(oldbits)[inspos]);
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

	nfa_t graph_reverse_nfa() const {
		nfa_t nfa(states.size());
		for (state_id_t s = 0; s < states.size(); ++s) {
			for_each_move(s, [&](state_id_t s, const Transition& t, char_t c) {
					assert(nfa[target_of(t)][c].empty());
					nfa[target_of(t)][c].push_back(s);
			});
		}
		return nfa;
	}

	// optimized
	//
	// create the inverse nfa, inverse of a general dfa is a nfa
    //
	// index[i] to index[i+1] are the index range of 'data'
	//   for target set of state 'i' in the inverse nfa
	CompactReverseNFA graph_reverse_nfa_opt() const {
		Automata<State, state_id_t> cnt; // counting size of nfa target set
		cnt.resize_states(states.size());
		// cnt[x][c] is the size of target set for (x, c) in the nfa
		for (state_id_t s = 0; s < states.size(); ++s) {
			for_each_move(s, [&](state_id_t s, const Transition& t, char_t c) {
				// 'n' is size of target set for (t, c) in the nfa
				state_id_t n = cnt.state_move(target_of(t), c);
				if (null_state == n)
					cnt.set_move(target_of(t), 1, c);
				else
					cnt.set_move(target_of(t), n+1, c);
			});
		}
		CompactReverseNFA nfa;
		nfa.index.resize(states.size() + 1); // the extra element is a guard
		size_t idx = 0;
		for (state_id_t s = 0; s < states.size(); ++s) {
			nfa.index[s] = idx; // start index of s in nfa.data
			cnt.for_each_dest(s, [&](state_id_t, state_id_t n){ idx += n; });
		}
		nfa.index.back() = idx; // last element is the guard
		nfa.data.resize_no_init(idx);

        // (ii[i]) is from 0 to (index[i+1] - index[i])
		valvec<int> ii(states.size(), 0);
		for (state_id_t s = 0; s < states.size(); ++s) {
			for_each_move(s, [&](state_id_t s, const Transition& t, char_t c) {
				state_id_t tt = target_of(t);
				nfa.data[nfa.index[tt] + ii[tt]] = CharTarget(c, s);
				ii[tt]++;
			});
		}
#if 0 // for test
        valvec<std::map<char_t, state_id_t> > cp(states.size());
        for (state_id_t s = 0; s < states.size(); ++s) {
            size_t low = nfa.index[s+0];
            size_t upp = nfa.index[s+1];
            assert(low <= upp);
            for (size_t i = low; i < upp; ++i) {
                CharTarget ct = nfa.data[i];
                cp[ct.t][ct.c] = s;
                assert(i == low || ct.c != nfa.data[i+1].c);
            }
        }
        for (state_id_t s = 0; s < states.size(); ++s) {
            for (int ch = 0; ch < 256; ++ch) {
                state_id_t t1 = target_of(state_move(s, ch));
                state_id_t t2 = cp[s].count(ch) ? cp[s][ch] : null_state;
                assert(t1 == t2);
            }
        }
#endif
		return nfa;
	}

	valvec<CharTarget> trie_reverse_dfa() const {
		valvec<CharTarget> inv(states.size(), CharTarget(-1, null_state));
		for (state_id_t s = 0; s < states.size(); ++s) {
			for_each_move(s, [&](state_id_t s, const Transition& t, char_t c) {
			//	assert(isprint(c) || isspace(c));
				CharTarget& ct = inv[target_of(t)];
				assert(-1 == ct.c);
				assert(null_state == ct.t);
				ct.c = c;
				ct.t = s;
			});
		}
		return inv;
	}

    // just walk the (curr, dest), don't access the 'ch'
    // this is useful such as when minimizing DFA
    template<class OP> // use ctz (count trailing zero)
    void for_each_dest(state_id_t curr, OP op) const {
        const State s = states[curr];
        if (s.is_single()) {
            state_id_t target = single_target(s);
            if (null_state != target)
                op(curr, first_trans(target, (Transition*)NULL));
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

    template<class OP> // use ctz (count trailing zero)
    void for_each_move(state_id_t curr, OP op) const {
		ASSERT_isNotFree(curr);
        const State s = states[curr];
        if (s.is_single()) {
			state_id_t target = single_target(s);
			if (null_state != target)
				op(curr, first_trans(target, (Transition*)NULL), s.getMinch());
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
				op(curr, first_trans(target, (Transition*)NULL), s.getMinch());
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
                op(curr, first_trans(target, (Transition*)NULL), s.getMinch());
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
	
	template<class QueueOrStack>
	class TreeWalker {
	public:
		TreeWalker(const Automata* au) {
		//	printf("%s\n", BOOST_CURRENT_FUNCTION);
			this->au = au;
			if (QueueOrStack_is_stack<QueueOrStack>::value)
				children.reserve(sigma);
		}

		void start() {
			assert(qBFS.empty());
			qBFS.push((state_id_t)initial_state);
		}

		state_id_t next() {
			assert(!qBFS.empty());
			state_id_t s = QueueOrStack_get(qBFS);
			qBFS.pop();
			ASSERT_isNotFree2(au, s);
			return s;
		}

		bool is_finished() const { return qBFS.empty(); }
		void putChildren(state_id_t curr) { putChildren_aux(curr, qBFS); }

	protected:
		template<class BaseCont>
		void putChildren_aux(state_id_t curr, std::stack<state_id_t, BaseCont>&) {
			// need to push to stack reversely
			au->for_each_dest(curr, [&](state_id_t, Transition t) {
				state_id_t target = target_of(t);
				children.unchecked_push_back(target);
			});
			while (!children.empty()) {
				qBFS.push(children.back());
				children.unchecked_pop_back();
			}
		}
		template<class BaseCont>
		void putChildren_aux(state_id_t curr, std::queue<state_id_t, BaseCont>&) {
			au->for_each_dest(curr, [&](state_id_t, Transition t) {
				state_id_t target = target_of(t);
				qBFS.push(target);
			});
		}

		const Automata*    au;
		// For DFS, the stack space is much larger than recursive DFS
		//          the stack space is proportional to BFS queue's
		QueueOrStack       qBFS; // queue for BFS, stack for DFS
		valvec<state_id_t> children; // only used for DFS
	};
	template<class QueueOrStack>
	class Walker {
	public:
		Walker(const Automata* au) {
		//	printf("%s\n", BOOST_CURRENT_FUNCTION);
			this->au = au;
			if (QueueOrStack_is_stack<QueueOrStack>::value)
				children.reserve(sigma);
		}

		void start() {
			if (mark.empty())
				mark.resize(au->total_states(), 0);
			else {
				assert(mark.size() >= au->total_states());
				mark.fill(0);
			}
			assert(qBFS.empty());
			qBFS.push((state_id_t)initial_state);
			mark.set1((state_id_t)initial_state);
		}

		state_id_t next() {
			assert(!qBFS.empty());
			state_id_t s = QueueOrStack_get(qBFS);
			qBFS.pop();
			assert(mark.is1(s));
			ASSERT_isNotFree2(au, s);
			return s;
		}

		bool is_finished() const { return qBFS.empty(); }
		void putChildren(state_id_t curr) { putChildren_aux(curr, qBFS); }

	protected:
		template<class BaseCont>
		void putChildren_aux(state_id_t curr, std::stack<state_id_t, BaseCont>&) {
			// need to push to stack reversely
			au->for_each_dest(curr, [&](state_id_t, Transition t) {
				state_id_t target = target_of(t);
				if (mark.is0(target)) {
					children.unchecked_push_back(target);
					mark.set1(target);
				}
			});
			while (!children.empty()) {
				qBFS.push(children.back());
				children.unchecked_pop_back();
			}
		}
		template<class BaseCont>
		void putChildren_aux(state_id_t curr, std::queue<state_id_t, BaseCont>&) {
			au->for_each_dest(curr, [&](state_id_t, Transition t) {
				state_id_t target = target_of(t);
				if (mark.is0(target)) {
					qBFS.push(target);
					mark.set1(target);
				}
			});
		}

		const Automata*    au;
        simplebitvec       mark;
		// For DFS, the stack space is much larger than recursive DFS
		//          the stack space is proportional to BFS queue's
		QueueOrStack       qBFS; // queue for BFS, stack for DFS
		valvec<state_id_t> children; // only used for DFS
	};
	typedef Walker<std::stack<state_id_t, std::vector<state_id_t> > > dfs_walker;
	typedef Walker<std::queue<state_id_t, std::deque <state_id_t> > > bfs_walker;

    template<class Visitor>
    class BFS_Tree_Vistor : Visitor {
        std::deque<state_id_t>* queue;
    public:
        void operator()(state_id_t source, state_id_t target, char_t ch) {
            Visitor::operator()(source, target, ch);
            queue->push_back(target);
        }
        BFS_Tree_Vistor(const Visitor& vis, std::deque<state_id_t>* queue)
          : Visitor(vis), queue(queue) {}
    };
    template<class Visitor>
    class BFS_Graph_Vistor : Visitor {
        std::deque<state_id_t>* queue;
        const bm_uint_t* bits;
    public:
        void operator()(state_id_t source, state_id_t target, char_t ch) {
            Visitor::operator()(source, target, ch);
            const int nb = sizeof(bm_uint_t) * 8;
            if (!(bits[target/nb] & bm_uint_t(1) << target%nb))
                queue->push_back(target);
        }
        BFS_Graph_Vistor(const Visitor& vis, std::deque<state_id_t>* queue,
                         const bm_uint_t* bits)
          : Visitor(vis), queue(queue), bits(bits) {}
    };
    enum class Color : char {
        White,
        Grey,
        Black
    };
    bool has_cycle_loop(state_id_t curr, Color* mark) const {
        if (Color::Grey == mark[curr])
            return true;
        if (Color::Black == mark[curr])
            return false;
        mark[curr] = Color::Grey;
        bool hasCycle = false;
        for_each_dest(curr, [=,&hasCycle](state_id_t, const Transition& t) {
            hasCycle = hasCycle || this->has_cycle_loop(target_of(t), mark);
        });
        mark[curr] = Color::Black;
        return hasCycle;
    }

	/// @return true if it is     a DAG from state curr
	///        false if it is not a DAG from state curr
    bool dag_pathes_loop(state_id_t curr, Color* mark
                       , size_t* paths_from_state) const {
	//	printf("curr=%ld\n", (long)curr);
		const size_t SIZE_T_MAX = std::numeric_limits<size_t>::max();
        assert(curr < (state_id_t)states.size());
        if (Color::Grey == mark[curr]) {
            paths_from_state[curr] = SIZE_T_MAX;
            paths_from_state[initial_state] = SIZE_T_MAX;
			assert(0);
            return false;
        }
        if (Color::Black == mark[curr]) {
            return true;
        }
        mark[curr] = Color::Grey;
        const State s = states[curr];
        size_t num = 0;
        if (s.is_single()) {
            state_id_t target = single_target(s);
            if (null_state != target) {
                if (!dag_pathes_loop(target, mark, paths_from_state))
					return false;
                num = paths_from_state[target];
			//	printf("--curr=%ld, target=%ld\n", (long)curr, (long)target);
                assert(num < SIZE_T_MAX);
            }
        } else {
			int bits = s.rlen(), nt;
            const transition_t* pt;
            if (is_32bitmap(bits)) {
                const header_b32& hb = pool.template at<header_b32>(s.getpos());
                nt = fast_popcount32(hb.b);
                pt = hb.s;
            } else {
                const header_max& hb = pool.template at<header_max>(s.getpos());
                nt = hb.popcnt_aligned(bits);
                pt = hb.base(bits);
            }
            for (int i = 0; i < nt; ++i) {
                assert(num < SIZE_T_MAX);
                state_id_t target = target_of(pt[i]);
                if (!dag_pathes_loop(target, mark, paths_from_state))
					return false;
                assert(paths_from_state[target] < SIZE_T_MAX);
                num += paths_from_state[target];
            }
        }
        if (s.is_term())
            num++;
        assert(num >= 1);
        assert(num < SIZE_T_MAX);
	//	printf("**curr=%ld, num=%ld\n", (long)curr, (long)num);
        paths_from_state[curr] = num;
        mark[curr] = Color::Black;
		return true;
    }

public:
    bool is_dag() const {
        valvec<Color> mark(states.size(), Color::White);
        return !has_cycle_loop(initial_state, &mark[0]);
    }
    // return ~size_t(0) indicate there is some loop in the state graph
    size_t dag_pathes() const {
		assert(num_used_states() >= 1);
		if (num_used_states() <= 1)
			return 0;
        valvec<Color> mark(states.size(), Color::White);
        valvec<size_t> paths_from_state(states.size());
        dag_pathes_loop(initial_state, &mark[0], &paths_from_state[0]);
        return paths_from_state[0];
    }

    // No mark bits, optimized for tree shape Automata
    template<class Visitor>
    void bfs_tree(Visitor vis) const {
        std::deque<state_id_t> queue;
        queue.push_back((state_id_t)initial_state);
        while (!queue.empty()) {
            state_id_t source = queue.front();
            queue.pop_front();
            for_each_move(source, BFS_Tree_Vistor<Visitor>(vis, &queue));
        }
    }

    template<class Visitor>
    void bfs_graph(Visitor vis) const {
        std::deque<state_id_t> queue;
        const int nb = sizeof(bm_uint_t) * 8;
        valvec<bm_uint_t> bits((states.size()+nb-1)/nb, 0);
        queue.push_back((state_id_t)initial_state);
        while (!queue.empty()) {
            state_id_t source = queue.front();
            queue.pop_front();
            bits[source/nb] |= bm_uint_t(1) << source%nb;
            BFS_Graph_Vistor<Visitor> gvis(vis, &queue, &bits[0]);
            for_each_move(source, gvis);
        }
    }

private:
	static FILE* fopen_output(const char* fname) {
		FILE* fp;
	   	if (strcmp(fname, "-") == 0 ||
			strcmp(fname, "/dev/fd/1") == 0 ||
			strcmp(fname, "/dev/stdout") == 0) {
				fp = stdout;
		} else {
			fp = fopen(fname, "w");
			if (NULL == fp) {
				fprintf(stderr, "%s:%d can not open \"%s\", err=%s\n"
					   , __FILE__, __LINE__, fname, strerror(errno));
				throw std::runtime_error(fname);
			}
		}
		return fp;
	}

	template<class OnNthWord> struct ForEachWord_DFS {
        const Automata* aut;
        std::string word;
		size_t      nth;
		OnNthWord on_nth_word;
        ForEachWord_DFS(const Automata* aut, OnNthWord op)
         : aut(aut), nth(0), on_nth_word(op) {
			enter(initial_state);
		}
		size_t enter(state_id_t tt) {
			size_t slen = 0;
			const State& ts = aut->states[tt];
			if (ts.is_pzip()) {
				size_t zpos = aut->get_zipped_path_str_pos(ts);
				const byte_t* s2 = &aut->pool.template at<byte_t>(zpos);
				slen = *s2++;
				word.append((const char*)s2, slen);
			}
			if (ts.is_term()) {
				on_nth_word(nth, word, tt);
				++nth;
			}
			return slen;
		}
        void operator()(state_id_t, const Transition& t, char_t ch) {
			word.push_back(ch);
			const state_id_t tt = target_of(t);
			const size_t slen = enter(tt) + 1;
			aut->template for_each_move<ForEachWord_DFS&>(tt, *this);
            word.resize(word.size() - slen); // pop_back
        }
    };
	template<class OnNthWord> friend struct ForEachWord_DFS;

public:
	/// @param on_nth_word(size_t nth, const std::string& word, state_id_t accept_state)
	///    calling of on_nth_word is in lexical_graphical order,
	///    nth is the ordinal, 0 based
	/// @returns  number of all words
	template<class OnNthWord>
	size_t for_each_word(OnNthWord on_nth_word) const {
		ForEachWord_DFS<OnNthWord> vis(this, on_nth_word);
        for_each_move<ForEachWord_DFS<OnNthWord>&>(initial_state, vis);
		return vis.nth;
	}

    size_t print_output(const char* fname = "-") const {
        printf("print_output(\"%s\")\n", fname);
		FILE* fp = fopen_output(fname);
        size_t cnt = for_each_word(
			[fp](size_t, const std::string& word, state_id_t) {
				fprintf(fp, "%s\n", word.c_str());
			});
		if (fp != stdout) fclose(fp);
		return cnt;
    }

	typedef smallmap<typename Hopcroft<state_id_t>::Splitter> ch_to_invset_t;

    template<class State2, class Trans2>
    void graph_dfa_minimize(Automata<State2, Trans2>& minimized) const {
        assert(states.size() < (size_t)null_state);
		nfa_t inv(graph_reverse_nfa());
        Hopcroft<state_id_t> H(states);
		H.refine([&inv](state_id_t curr, ch_to_invset_t& mE) {
			for (const auto& csv : inv[curr]) {
				char_t c = csv.first;
				const auto& sv = csv.second;
				for (state_id_t s : sv)
					mE.bykey(c).insert(s); // Generate {(c,{inverse(curr)})}
			}
		});
		inv.clear();
		collapse(H, minimized);
    }
    template<class State2, class Trans2>
    void graph_dfa_minimize_opt(Automata<State2, Trans2>& minimized) const {
        assert(states.size() < (size_t)null_state);
		CompactReverseNFA inv(graph_reverse_nfa_opt());
        Hopcroft<state_id_t> H(states);
		H.refine([&inv](state_id_t curr, ch_to_invset_t& mE) {
			size_t low = inv.index[curr+0];
			size_t upp = inv.index[curr+1];
			for (size_t i = low; i < upp; ++i) {
				CharTarget  ct = inv.data[i];
				mE.bykey(ct.c).insert(ct.t); // Generate {(c,{inverse(curr)})}
			}
		});
		inv.clear();
		collapse(H, minimized);
    }
    template<class State2, class Trans2>
    void trie_dfa_minimize(Automata<State2, Trans2>& minimized) const {
        assert(states.size() < (size_t)null_state);
		valvec<CharTarget> inv(trie_reverse_dfa());
        Hopcroft<state_id_t> H(states);
		H.refine([&inv](state_id_t curr, ch_to_invset_t& mE) {
			if (initial_state != curr) {
				CharTarget ct = inv[curr];
				mE.bykey(ct.c).insert(ct.t);
			}
		});
		inv.clear();
		collapse(H, minimized);
    }
    template<class State2, class Trans2>
    void collapse(Hopcroft<state_id_t>& H,
				  Automata<State2, Trans2>& minimized) const
   	{
		size_t min_size = H.P.size();
		H.P.clear(); // H.P is no longer used
        minimized.resize_states(min_size);
		for (state_id_t s = 0; s < states.size(); ++s) {
			for_each_move(s, [&](state_id_t s, const Transition& t, char_t c) {
    			const typename State2::state_id_t old =
					minimized.add_move(H.L[s].blid, H.L[target_of(t)].blid, c);
				assert(State2::null_state == old || old == H.L[target_of(t)].blid);
			});
            if (states[s].is_term())
                minimized.set_term_bit(H.L[s].blid);
		}
	}
    void graph_dfa_minimize() {
        Automata minimized;
        graph_dfa_minimize(minimized);
        this->swap(minimized);
    }
    void graph_dfa_minimize_opt() {
        Automata minimized;
        graph_dfa_minimize_opt(minimized);
        this->swap(minimized);
    }
    void trie_dfa_minimize() {
        Automata minimized;
        trie_dfa_minimize(minimized);
        this->swap(minimized);
    }

    void resize_states(size_t new_states_num) {
		if (new_states_num >= max_state)
			throw std::logic_error("Automata::resize_states: exceed max_state");
        states.resize(new_states_num);
    }

    size_t mem_size() const {
        return pool.size() + sizeof(State) * states.size();
    }
	size_t free_size() const { return pool.free_size(); }

	void shrink_to_fit() {
		states.shrink_to_fit();
		pool.shrink_to_fit();
	}

protected:
	virtual void dot_write_one_state(FILE* fp, long ls) const {
		if (this->states[ls].is_pzip()) {
			size_t zpos = this->get_zipped_path_str_pos(this->states[ls]);
			const byte_t* s2 = &this->pool.template at<byte_t>(zpos);
			int n = *s2++;
			if (states[ls].is_term())
				fprintf(fp, "\tstate%ld[label=\"%ld:%.*s\" shape=\"doublecircle\"];\n", ls, ls, n, s2);
			else
				fprintf(fp, "\tstate%ld[label=\"%ld:%.*s\"];\n", ls, ls, n, s2);
		}
		else {
			if (states[ls].is_term())
				fprintf(fp, "\tstate%ld[label=\"%ld\" shape=\"doublecircle\"];\n", ls, ls);
			else
				fprintf(fp, "\tstate%ld[label=\"%ld\"];\n", ls, ls);
		}
	}
	virtual void dot_write_one_transition(FILE* fp, long s, const Transition& t, char_t ch) const {
		long target = (long)target_of(t);
		fprintf(fp, "\tstate%ld -> state%ld [label=\"%c\"];\n",	s, target, (int)ch);
	}
public:
    void write_dot_file(const std::string& fname) const { write_dot_file(fname.c_str()); }
    void write_dot_file(const char* fname) const {
        FILE* dot = fopen(fname, "w");
        if (dot) {
            write_dot_file(dot);
            fclose(dot);
        } else {
            fprintf(stderr, "can not open %s for write\n", fname);
        }
    }
    virtual void write_dot_file(FILE* fp) const {
	//	printf("%s:%d %s\n", __FILE__, __LINE__, BOOST_CURRENT_FUNCTION);
		fprintf(fp, "digraph G {\n");
		dfs_walker dfs(this);
   		dfs.start();
		while (!dfs.is_finished()) {
			state_id_t s = dfs.next();
			dot_write_one_state(fp, s);
			dfs.putChildren(s);
        }
		dfs.start();
		while (!dfs.is_finished()) {
			long i = dfs.next();
            for_each_move(i, [fp,this](state_id_t s, const Transition& t, char_t c) {
				this->dot_write_one_transition(fp, s, t, c);
            });
			dfs.putChildren((state_id_t)i);
        }
		fprintf(fp, "}\n");
    }

	virtual ~Automata() {}

    void swap(Automata& y) {
        pool  .swap(y.pool  );
        states.swap(y.states);
		std::swap(firstFreeState, y.firstFreeState);
		std::swap(numFreeStates , y.numFreeStates );
        std::swap(transition_num, y.transition_num);
        std::swap(zpath_states     , y.zpath_states     );
    }

    ///@{
    ///@param eosMark End Of String Mark
    bool add_word_eos(const std::string& key, char_t eosMark) {
        assert(!key.empty()); // don't allow empty key
        return add_word_eos(key.data(), (int)key.size(), eosMark);
    }
    bool add_word_eos(const char* key, char_t eosMark) {
        size_t len = strlen(key);
        assert(len > 0); // don't allow empty key
        return add_word_eos(key, len, eosMark);
    }
    bool add_word_eos(const char* key, size_t len, char_t eosMark) {
        assert(len > 0); // don't allow empty key
        state_id_t curr = initial_state;
        state_id_t next;
        size_t j = 0;
        for (;j < len; ++j) {
            next = target_of(state_move(curr, key[j]));
            if (null_state == next)
                goto AddNewStates;
            assert(next < (state_id_t)states.size());
            curr = next;
        }
        next = target_of(state_move(curr, eosMark));
        if (null_state == next)
            goto AddEos;
        if (states[next].is_term()) {
            return false; // existed
        } else {
            assert(0);
            throw std::logic_error("unexpected");
        }
    AddNewStates:
        do {
            next = this->new_state();
            this->add_move_checked(curr, next, key[j]);
            curr = next;
            ++j;
        } while (j < len);
   AddEos:
        next = this->new_state();
        this->add_move_checked(curr, next, eosMark);
        states[next].set_term_bit();
        return true;
    }

    bool add_word(const std::string& key) {
        assert(!key.empty()); // don't allow empty key
        return add_word(key.data(), (int)key.size());
    }
    bool add_word(const char* key) {
        size_t len = strlen(key);
        assert(len > 0); // don't allow empty key
        return add_word(key, len);
    }
    bool add_word(const char* key, size_t len) {
		assert(!zpath_states);
        assert(len > 0); // don't allow empty key
        state_id_t curr = initial_state;
        state_id_t next;
        size_t j = 0;
        while (j < len) {
            next = target_of(state_move(curr, key[j]));
            if (null_state == next)
                goto AddNewStates;
            assert(next < (state_id_t)states.size());
            curr = next;
            ++j;
        }
        // key could be the prefix of some existed key word
        if (states[curr].is_term()) {
            return false; // existed
        } else {
            states[curr].set_term_bit();
            return true;
        }
    AddNewStates:
        do {
            next = this->new_state();
            this->add_move_checked(curr, next, key[j]);
            curr = next;
            ++j;
        } while (j < len);
        // now curr == next, and it is a Terminal State
        states[curr].set_term_bit();
        return true;
    }
    //@}
	
	///@param out path
	///@return
	///  - null_state   : the str from 'from' to 'to' is newly created
	///  - same as 'to' : the str from 'from' to 'to' has existed
	///  - others       : the str from 'from' to 'XX' has existed, XX is not 'to'
	///                   in this case, the Automata is unchanged
	///@note
	///  - This function don't change the Automata if str from 'from' to 'to'
	///    has existed. The caller should check the return value if needed.
	state_id_t make_path( state_id_t from, state_id_t to
						, const char* str, size_t len
						, valvec<state_id_t>* path) {
		assert(!zpath_states);
		assert(null_state != from);
		assert(null_state != to);
		assert(NULL != path);
		assert(NULL != str);
	#ifdef NDEBUG
		if (0 == len) return false;
	#else
        assert(len > 0); // don't allow empty str
	#endif
		auto& p = *path;
		p.resize(len+1);
        p[0] = from;
        for (size_t j = 0; j < len-1; ++j) {
            if (null_state == (p[j+1] = target_of(state_move(p[j], str[j])))) {
				do add_move(p[j], p[j+1] = new_state(), str[j]);
				while (++j < len-1);
				break;
			}
            assert(p[j+1] < (state_id_t)states.size());
        }
		state_id_t old_to = this->add_move(p[len-1], to, str[len-1]);
		p[len] = (null_state == old_to) ? to : old_to;
		return old_to;
	}

	// dead states: states which are unreachable to any final states
	void remove_dead_states() {
		Automata clean;
		remove_dead_states(clean);
		this->swap(clean);
	}
	template<class CleanDFA>
	void remove_dead_states(CleanDFA& clean) {
		CompactReverseNFA inv(graph_reverse_nfa_opt());
		valvec<state_id_t> stack;
		simplebitvec mark(states.size(), false);
		if (numFreeStates) {
			simplebitvec Free(states.size(), false);
			for (state_id_t f = firstFreeState; f != null_state; ) {
				Free.set1(f);
				f = states[f].get_target();
			}
			for (state_id_t root = 0; root < states.size(); ++root) {
				if (Free.is0(root) && states[root].is_term())
					stack.push_back(root), mark.set1(root);
			}
		} else { // has no any free state
			for (state_id_t root = 0; root < states.size(); ++root) {
				if (states[root].is_term())
					stack.push_back(root), mark.set1(root);
			}
		}
		// DFS search from multiple final states
		while (!stack.empty()) {
			state_id_t parent = stack.back(); stack.pop_back();
			size_t beg = inv.index[parent+0];
			size_t end = inv.index[parent+1];
			assert(beg <= end);
			for (size_t i = beg; i < end; ++i) {
				state_id_t child = inv.data[i].t;
				assert(child < states.size());
				if (mark.is0(child))
					stack.push_back(child), mark.set1(child);
			}
		}
		size_t new_au_states = 0;
		typedef typename CleanDFA::state_id_t state_id_t2;
		valvec<state_id_t2> map(states.size(), (state_id_t2)clean.null_state);
		for (size_t s = 0; s < states.size(); ++s) {
			if (mark.is1(s))
				map[s] = new_au_states++;
		}
		clean.resize_states(new_au_states);
		for (size_t s = 0; s < states.size(); ++s) {
			if (map[s] == null_state)
			   	continue;
			for_each_move(s, [&](state_id_t src, Transition dst, char_t ch) {
				if (map[target_of(dst)] != clean.null_state) {
					state_id_t2 src2 = map[src];
					state_id_t2 dst2 = map[target_of(dst)];
					clean.add_move(src2, dst2, ch);
				}
			});
			if (states[s].is_term())
				clean.states[map[s]].set_term_bit();
		}
	}

#ifdef ENABLE_DUP_PATH_POOL
	struct ZpoolEntry {
		size_t cnt;
		size_t pos;
		const static size_t null_pos = 1;
		ZpoolEntry() : cnt(0), pos(null_pos) {}
	};
#endif
	template<class QueueOrStack>
	class ZipWalker : public Walker<QueueOrStack> {
		const bit_access  is_confluence; // in-degree
	public:
        valvec<state_id_t> path; // state_move(path[j], strp[j]) == path[j+1]
        valvec<char_t>     strp;

		ZipWalker(const Automata* au, const bit_access is_confluence)
	   	 : Walker<QueueOrStack>(au), is_confluence(is_confluence)
	   	{}

		void getOnePath() {
			const Automata* au = this->au;
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
				assert(this->mark.is0(i));
			// 	this->mark.set1(i); // not needed
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
			compute_indegree(indg);
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
			path_zip<ZippedAutomata, std::queue<state_id_t> >(is_confluence, zipped, s2ds);
		else if (strcasecmp(BFS_or_DFS, "DFS") == 0)
			path_zip<ZippedAutomata, std::stack<state_id_t> >(is_confluence, zipped, s2ds);
		else
			throw std::invalid_argument("Automata::path_zip: must be DFS or BFS");
	}
    template<class ZippedAutomata, class QueueOrStack>
    void path_zip(const bit_access is_confluence, ZippedAutomata& zipped,
				  valvec<typename ZippedAutomata::state_id_t>& s2ds) const {
        assert(zipped.total_states() == 1);
		assert(!zpath_states);
		ASSERT_isNotFree(initial_state);
        const size_t min_compress_path_len = 3;
#if defined(NDEBUG)
	   	s2ds.resize_no_init(states.size());
#else
	   	s2ds.resize(states.size(), ZippedAutomata::null_state+0);
#endif
		IF_DUP_PATH_POOL(hash_strmap<ZpoolEntry> zwc;)
		ZipWalker<QueueOrStack> walker(this, is_confluence);
		{	size_t ds = 0;
			walker.start();
			while (!walker.is_finished()) {
				walker.getOnePath();
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
				walker.putChildren(walker.path.back());
			}
			zipped.resize_states(ds);
	   	}
		IF_DUP_PATH_POOL(zwc.sort_fast();)
		walker.start();
        while (!walker.is_finished()) {
			walker.getOnePath();
			typename ZippedAutomata::state_id_t zs = s2ds[walker.path.back()];
			this->for_each_move(walker.path.back(),
			  [&](state_id_t s1, transition_t t, char_t c) {
				zipped.add_move_checked(zs, s2ds[target_of(t)], c);
			});
            if (walker.path.size() >= min_compress_path_len) {
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
			walker.putChildren(walker.path.back());
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
		zipped.path_zip_freeze();
	}

	void path_zip_freeze() {
   		pool.shrink_to_fit();
		zpath_states = true;
	}

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

    void compute_indegree(valvec<state_id_t>& in) const {
        in.resize_no_init(states.size());
        in.fill(0);
		dfs_walker dfs(this);
		dfs.start();
		while (!dfs.is_finished()) {
			state_id_t i = dfs.next();
            this->for_each_dest(i, [&](state_id_t, transition_t t){
                in[target_of(t)]++;
            });
			dfs.putChildren(i);
        }
    }

	IF_DUP_PATH_POOL(bool   allow_zpath_pool;)

	enum { SERIALIZATION_VERSION = 1 };

	template<class DataIO> void load_au(DataIO& dio) {
		typename DataIO::my_var_uint64_t n_pool, n_states;
		typename DataIO::my_var_uint64_t n_firstFreeState, n_numFreeStates;
		byte_t b_zpath_states;
		dio >> b_zpath_states;
		dio >> n_pool;
		dio >> n_states;
		dio >> n_firstFreeState;
		dio >> n_numFreeStates;
		zpath_states = b_zpath_states;
		pool  .clear();
		states.clear();
		pool  .resize_no_init(n_pool.t);
		states.resize_no_init(n_states.t);
		dio.ensureRead(&pool.template at<byte_t>(0), n_pool.t);
		dio.ensureRead(&states[0], sizeof(State)*n_states.t);
	}
	template<class DataIO> void save_au(DataIO& dio) const {
		dio << (byte_t)zpath_states;
		dio << typename DataIO::my_var_uint64_t(pool  .size());
		dio << typename DataIO::my_var_uint64_t(states.size());
		dio << typename DataIO::my_var_uint64_t(firstFreeState);
		dio << typename DataIO::my_var_uint64_t(numFreeStates);
		dio.ensureWrite(&pool.template at<byte_t>(0), pool.size());
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
			typedef typename DataIO::my_DataFormatException bad_format;
			std::string msg("state_name[data=" + state_name
								  + " @ code=" + typeid(State).name()
								  + "]");
			throw bad_format(msg);
		}
		au.load_au(dio);
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
	bool   zpath_states;
};

#endif // __automata_penglei__

