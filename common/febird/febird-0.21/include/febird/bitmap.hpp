#ifndef __penglei_bitmap_h__
#define __penglei_bitmap_h__

#include <assert.h>
#include <limits.h>
#include "valvec.hpp"

#if defined(_MSC_VER)
// if some other headers fucked this 2 typedef in MSVC,
// comment the following 2 lines:
    typedef unsigned __int64 uint64_t;
    typedef unsigned int     uint32_t;
    typedef unsigned short   uint16_t;
#else
    #include <stdint.h>
#endif


// select best popcount implementations
//
#if defined(_MSC_VER) && _MSC_VER >= 1500 // VC 2008
//  different popcount instructions in different headers
//  #include <nmmintrin.h> // for _mm_popcnt_u32, _mm_popcnt_u64
    #include <intrin.h> // for __popcnt16, __popcnt, __popcnt64

    #pragma intrinsic(_BitScanForward)
    #define fast_popcount32 __popcnt
    #if defined(_WIN64) || defined(_M_X64) || defined(_M_IA64)
//      #define fast_popcount64 _mm_popcnt_u64
        #define fast_popcount64 __popcnt64
        #pragma intrinsic(_BitScanForward64)
    #else
        inline int fast_popcount64(unsigned __int64 x)
        {
            return
//              _mm_popcnt_u32((unsigned int)(x      )) +
//              _mm_popcnt_u32((unsigned int)(x >> 32));
                __popcnt((unsigned)(x      )) +
                __popcnt((unsigned)(x >> 32));
        }
    #endif
        inline int fast_ctz32(uint32_t x) {
            assert(0 != x);
            unsigned long c;
            _BitScanForward(&c, x);
            return c;
        }
        inline int fast_ctz64(uint64_t x) {
            assert(0 != x);
            unsigned long c = 0;
        #if defined(_WIN64) || defined(_M_X64) || defined(_M_IA64)
            _BitScanForward64(&c, x);
        #else
            unsigned long x1 = (unsigned long)(x & 0xFFFFFFFF);
            unsigned long x2 = (unsigned long)(x >> 32);
            if (x1) {
                _BitScanForward(&c, x1);
            } else {
                if (_BitScanForward(&c, x2))
	                c += 32;
				else
					c = 64; // all bits are zero
            }
        #endif
            return c;
        }
#elif defined(__GNUC__) && __GNUC__ >= 4 || \
      defined(__INTEL_COMPILER) && __INTEL_COMPILER >= 900 || \
      defined(__clang__)
    #define fast_popcount32 __builtin_popcount
    #define fast_ctz32      __builtin_ctz
    #ifdef ULONG_MAX
        #if ULONG_MAX == 0xFFFFFFFF
            inline int fast_popcount64(unsigned long long x) {
                return __builtin_popcount((unsigned)(x      )) +
                       __builtin_popcount((unsigned)(x >> 32));
            }
            #define fast_ctz64      __builtin_ctzll
        #else
            #define fast_popcount64 __builtin_popcountl
            #define fast_ctz64      __builtin_ctzl
        #endif
    #else
        #error "ULONG_MAX must be defined, use #include <limits.h>"
    #endif
#else // unknow or does not support popcount
    static const char a_popcount_bits[256] = {
        0, 1, 1, 2, 1, 2, 2, 3, 1, 2, 2, 3, 2, 3, 3, 4,
        1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5,
        1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5,
        2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
        1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5,
        2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
        2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
        3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,
        1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5,
        2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
        2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
        3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,
        2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
        3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,
        3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,
        4, 5, 5, 6, 5, 6, 6, 7, 5, 6, 6, 7, 6, 7, 7, 8,
    };
    inline int fast_popcount32(uint32_t x)
    {
        return
            a_popcount_bits[255 & x    ] +
            a_popcount_bits[255 & x>> 8] +
            a_popcount_bits[255 & x>>16] +
            a_popcount_bits[255 & x>>24] ;
    }
    inline int fast_popcount64(uint64_t x)
    {
        return
            a_popcount_bits[255 & x    ] +
            a_popcount_bits[255 & x>> 8] +
            a_popcount_bits[255 & x>>16] +
            a_popcount_bits[255 & x>>24] +

            a_popcount_bits[255 & x>>32] +
            a_popcount_bits[255 & x>>40] +
            a_popcount_bits[255 & x>>48] +
            a_popcount_bits[255 & x>>56] ;
    }
#endif // select best popcount implementations

typedef size_t bm_uint_t;

inline int fast_popcount(uint32_t x) { return fast_popcount32(x); }
inline int fast_popcount(uint64_t x) { return fast_popcount64(x); }

inline int fast_popcount_trail(uint32_t x, int n) { return fast_popcount32(x & ~(~uint32_t(0) << n)); }
inline int fast_popcount_trail(uint64_t x, int n) { return fast_popcount64(x & ~(~uint64_t(0) << n)); }

inline int fast_ctz(uint32_t x) { return fast_ctz32(x); }
inline int fast_ctz(uint64_t x) { return fast_ctz64(x); }

template<int Bits> class BitsToUint;
template<> class BitsToUint<32> { typedef uint32_t type; };
template<> class BitsToUint<64> { typedef uint64_t type; };

template<int TotalBitsN, class BlockT = bm_uint_t>
class BitMap {
public:
    enum { BlockBits = sizeof(BlockT) * 8 };
    enum { TotalBits = (TotalBitsN + BlockBits - 1) & ~(BlockBits - 1)};
    enum { BlockN = TotalBits / sizeof(BlockT) / 8};
    enum { BlockCount = BlockN };
    typedef BlockT block_t;
protected:
    BlockT  bm[BlockN];
public:
    explicit BitMap(bool initial_value) { fill(initial_value); }
	void fill(bool value) {
        const BlockT e = value ? ~BlockT(0) : 0;
        for (int i = 0; i < BlockN; ++i) bm[i] = e;
	}

    BlockT  block(int i) const { return bm[i]; }
    BlockT& block(int i)       { return bm[i]; }

	BitMap& operator+=(const BitMap& y) {
		// same as  |=
        for (int i = 0; i < BlockN; ++i) bm[i] |=  y.bm[i];
		return *this;
	}
	BitMap& operator-=(const BitMap& y) {
        for (int i = 0; i < BlockN; ++i) bm[i] &= ~y.bm[i];
		return *this;
	}
	BitMap& operator^=(const BitMap& y) {
        for (int i = 0; i < BlockN; ++i) bm[i] ^= y.bm[i];
		return *this;
	}
	BitMap& operator&=(const BitMap& y) {
        for (int i = 0; i < BlockN; ++i) bm[i] &= y.bm[i];
		return *this;
	}
	BitMap& operator|=(const BitMap& y) {
        for (int i = 0; i < BlockN; ++i) bm[i] |= y.bm[i];
		return *this;
	}

    BitMap& operator<<=(int n) { shl(n); return *this; }
    void shl(int n, int realBlocks = BlockN) { // shift bit left
        assert(realBlocks <= BlockN);
        assert(n < BlockBits * realBlocks);
        assert(n > 0);
        int c = n / BlockBits;
        if (c > 0) {
            // copy backward, expected to be loop unrolled
            // when this function is calling by shl(n)
            for (int i = realBlocks-1; i >= c; --i)
                bm[i] = bm[i-c];
            memset(bm, 0, sizeof(BlockT)*c);
        }
        if ((n %= BlockBits) != 0) {
            BlockT r = bm[c] >> (BlockBits - n);
            bm[c] <<= n;
            for (int i = c+1; i < realBlocks; ++i) {
                BlockT r2 = bm[i] >> (BlockBits - n);
                bm[i] <<= n;
                bm[i]  |= r;
                r = r2;
            }
        }
    }
    void set1(int n) {
        assert(n >= 0);
        assert(n < TotalBits);
        bm[n/BlockBits] |=  (BlockT(1) << (n%BlockBits));
    }
    void set0(int n) {
        assert(n >= 0);
        assert(n < TotalBits);
        bm[n/BlockBits] &= ~(BlockT(1) << (n%BlockBits));
    }
	bool operator[](int n) { // alias of 'is1'
        assert(n >= 0);
        assert(n < TotalBits);
        return (bm[n/BlockBits] & (BlockT(1) << (n%BlockBits))) != 0;
	}
    bool is1(int n) const {
        assert(n >= 0);
        assert(n < TotalBits);
        return (bm[n/BlockBits] & (BlockT(1) << (n%BlockBits))) != 0;
    }
    bool is0(int n) const {
        assert(n >= 0);
        assert(n < TotalBits);
        return (bm[n/BlockBits] & (BlockT(1) << (n%BlockBits))) == 0;
    }
	bool is_all0() const {
		for (int i = 0; i < BlockN; ++i)
			if (bm[i])
				return false;
		return true;
	}
	bool is_all1() const {
		for (int i = 0; i < BlockN; ++i)
			if (BlockT(-1) != bm[i])
				return false;
		return true;
	}
	bool has_any0() const { return !is_all1(); }
	bool has_any1() const { return !is_all0(); }

    int popcnt() const {
        int c = 0; // small BlcokN will triger loop unroll optimization
        for (int i = 0; i < BlockN; ++i) c += fast_popcount(bm[i]);
        return c;
    }

    /// popcnt in blocks [0, align_bits(bits)/BlockBits)
    int popcnt_aligned(int bits) const {
        assert(bits >= 1);
        assert(bits <= TotalBits);
        // align to block boundary
        int n = (bits + BlockBits-1) / BlockBits;
        int c = 0; // small BlcokN will trigger loop unroll optimization
        for (int i = 0; i < BlockN; ++i) {
            if (i < n)
                c += fast_popcount(bm[i]);
            else
                break;
        }
        return c;
    }

	/// popcnt in bits: [0, bitpos), not counting bm[bitpos]
    int popcnt_index(int bitpos) const {
        assert(bitpos >= 0);
        assert(bitpos < TotalBits);
	//	assert(is1(bitpos)); // often holds, but not required to always holds
        int n = bitpos/BlockBits;
        int c = 0; // small BlcokN will trigger loop unroll optimization
        for (int i = 0; i < BlockN; ++i) {
            if (i < n)
                c += fast_popcount(bm[i]);
            else
                break;
        }
		if (int shift = bitpos % BlockBits)
        	c += fast_popcount_trail(bm[n], shift);
        return c;
    }

    static int align_bits(int n) {
        return (n + BlockBits-1) & ~(BlockBits-1);
    }
};

class simplebitvec {
    valvec<bm_uint_t> bl;
    static const size_t nb = sizeof(bm_uint_t)*8;
public:
	static size_t bits_to_blocks(size_t nbits) { return (nbits+nb-1)/nb; }
	static size_t align_bits(size_t nbits) { return (nbits+nb-1) & ~(nb-1); }

    simplebitvec() {}
    explicit simplebitvec(size_t bits, bool val = false);
    simplebitvec(size_t bits, bool val, bool padding);
	void clear();
	void erase_all() { bl.erase_all(); }
	void fill(bool val);
    void resize(size_t newsize, bool val=false);
	void resize_no_init(size_t newsize);
	void resize_fill(size_t newsize, bool val=false);

	simplebitvec& operator-=(const simplebitvec& y);
	simplebitvec& operator^=(const simplebitvec& y);
	simplebitvec& operator&=(const simplebitvec& y);
	simplebitvec& operator|=(const simplebitvec& y);
	void block_or(const simplebitvec& y, size_t yblstart, size_t blcnt);
	void block_and(const simplebitvec& y, size_t yblstart, size_t blcnt);

    bool operator[](size_t i) const { // alias of 'is1'
        assert(i < bl.size()*nb);
        return (bl[i/nb] & bm_uint_t(1) << i%nb) != 0;
    }
    bool is1(size_t i) const {
        assert(i < bl.size()*nb);
        return (bl[i/nb] & bm_uint_t(1) << i%nb) != 0;
    }
    bool is0(size_t i) const {
        assert(i < bl.size()*nb);
        return (bl[i/nb] & bm_uint_t(1) << i%nb) == 0;
    }
	void set(size_t i, bool val) {
        assert(i < bl.size()*nb);
		val ? set1(i) : set0(i);
	}
    void set0(size_t i) {
        assert(i < bl.size()*nb);
        bl[i/nb] &= ~(bm_uint_t(1) << i%nb);
    }
    void set1(size_t i) {
        assert(i < bl.size()*nb);
        bl[i/nb] |= bm_uint_t(1) << i%nb;
    }
//	void set0(size_t i, size_t nbits);
//	void set1(size_t i, size_t nbits);
	void block_set0(size_t bit_idx) {
        assert(bit_idx < bl.size()*nb);
        bl[bit_idx/nb] = 0;
	}
	bool isall0() const;
   	bool isall1() const;
	size_t popcnt() const;
	size_t popcnt(size_t blstart, size_t blcnt) const;

	size_t blsize() const { return bl.size(); }
	size_t bytes() const { return sizeof(bm_uint_t) * bl.size(); }
	size_t size() const { return bl.size() * nb; }
	bool  empty() const { return bl.empty(); }
	void  swap(simplebitvec& y) { bl.swap(y.bl); }

	valvec<bm_uint_t>& get_blvec() { return bl; }
	const valvec<bm_uint_t>& get_blvec() const { return bl; }

	      bm_uint_t* bldata()       { return &bl[0]; }
	const bm_uint_t* bldata() const { return &bl[0]; }
};

class bit_access {
    bm_uint_t* p;
    static const size_t nb = sizeof(bm_uint_t)*8;
public:
    static size_t blocks(size_t bits) {
        return (bits + nb-1) / nb;
    }
    bool operator[](size_t i) const { // alias of 'is1'
        return (p[i/nb] & bm_uint_t(1) << i%nb) != 0;
    }
    bool is1(size_t i) const {
        return (p[i/nb] & bm_uint_t(1) << i%nb) != 0;
    }
    bool is0(size_t i) const {
        return (p[i/nb] & bm_uint_t(1) << i%nb) == 0;
    }
    void set0(size_t i) {
        p[i/nb] &= ~(bm_uint_t(1) << i%nb);
    }
    void set1(size_t i) {
        p[i/nb] |= bm_uint_t(1) << i%nb;
    }
    bit_access(bm_uint_t* p) : p(p) {}
};


#endif

