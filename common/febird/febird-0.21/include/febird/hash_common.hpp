#ifndef __febird_fast_hash_common__
#define __febird_fast_hash_common__

#include "node_layout.hpp"
#include <algorithm>

// HSM_ : Hash String Map
#define HSM_SANITY assert

#if defined(__GNUC__) || defined(__INTEL_COMPILER) || defined(__clang__)
	#define HSM_unlikely(expr) __builtin_expect(expr, 0)
#else
	#define HSM_unlikely(expr) expr
#endif

#if 0
#include <boost/type_traits/is_unsigned.hpp>
template<class Uint>
inline Uint BitsRotateLeft(Uint x, int c) {
	assert(c >= 0);
	assert(c < (int)sizeof(Uint)*8);
	BOOST_STATIC_ASSERT(Uint(~Uint(0)) > 0); // is unsigned
	BOOST_STATIC_ASSERT(boost::is_unsigned<Uint>::value);
	return x << c | x >> (sizeof(Uint)*8 - c);
}
template<class Uint>
inline Uint BitsRotateRight(Uint x, int c) {
	assert(c >= 0);
	assert(c < (int)sizeof(Uint)*8);
	BOOST_STATIC_ASSERT(Uint(~Uint(0)) > 0); // is unsigned
	BOOST_STATIC_ASSERT(boost::is_unsigned<Uint>::value);
	return x >> c | x << (sizeof(Uint)*8 - c);
}
#else
	#define BitsRotateLeft(x,c)  ((x) << (c) | (x) >> (sizeof(x)*8 - (c)))
	#define BitsRotateRight(x,c) ((x) >> (c) | (x) << (sizeof(x)*8 - (c)))
#endif

inline static size_t
__hsm_stl_next_prime(size_t __n)
{
	static const size_t primes[] =
	{
		5,11,19,37,   53ul,         97ul,         193ul,       389ul,
		769ul,        1543ul,       3079ul,       6151ul,      12289ul,
		24593ul,      49157ul,      98317ul,      196613ul,    393241ul,
		786433ul,     1572869ul,    3145739ul,    6291469ul,   12582917ul,
		25165843ul,   50331653ul,   100663319ul,  201326611ul, 402653189ul,
		805306457ul,  1610612741ul, 3221225473ul, 4294967291ul
	};
	const size_t* __first = primes;
	const size_t* __last = primes + sizeof(primes)/sizeof(primes[0]);
	const size_t* pos = std::lower_bound(__first, __last, __n);
	return pos == __last ? __last[-1] : *pos;
}

inline size_t __hsm_align_pow2(size_t x) {
	assert(x > 0);
	size_t p = 0, y = x;
	while (y)
		y >>= 1, ++p;
	if ((size_t(1) << (p-1)) == x)
		return x;
	else
		return size_t(1) << p;
}

template<class UInt>
struct dummy_bucket{
	typedef UInt link_t;
	static const UInt tail = ~UInt(0);
	static const UInt delmark = tail-1;
	static UInt b[1];
};
template<class UInt> UInt dummy_bucket<UInt>::b[1] = {~UInt(0)};

template<class Key, class Hash, class Equal>
struct hash_and_equal : private Hash, private Equal {
	size_t hash(const Key& x) const { return Hash::operator()(x); }
	bool  equal(const Key& x, const Key& y) const {
	   	return Equal::operator()(x, y);
   	}
	hash_and_equal() {}
	hash_and_equal(const Hash& h, const Equal& eq) : Hash(h), Equal(eq) {}
};

struct HSM_DefaultDeleter { // delete NULL is OK
	template<class T> void operator()(T* p) const { delete p; }
};

template<class HashMap, class ValuePtr, class Deleter>
class febird_ptr_hash_map : private Deleter {
	HashMap map;
//	typedef void (type_safe_bool*)(febird_ptr_hash_map**, Deleter**); 
//	static void type_safe_bool_true(febird_ptr_hash_map**, Deleter**) {}
public:
	typedef typename HashMap::  key_type   key_type;
	typedef ValuePtr value_type;
//	BOOST_STATIC_ASSERT(boost::mpl::is_pointer<value_type>);

	~febird_ptr_hash_map() { del_all(); }

	value_type operator[](const key_type& key) const {
		size_t idx = map.find_i(key);
		return idx == map.end_i() ? NULL : map.val(idx);
	}

	bool is_null(const key_type& key) const { return (*this)[key] == NULL; }

	void replace(const key_type& key, value_type pval) {
		std::pair<size_t, bool> ib = map.insert_i(key, pval);
		if (!ib.second) {
			value_type& old = map.val(ib.first);
			if (old) static_cast<Deleter&>(*this)(old);
			old = pval;
		}
	}
	std::pair<value_type*, bool> insert(const key_type& key, value_type pval) {
		std::pair<size_t, bool> ib = map.insert_i(key, pval);
		return std::make_pair(&map.val(ib.first), ib.second);
	}
	void clear() {
		del_all();
		map.clear();
	}
	void erase_all() {
		del_all();
	   	map.erase_all();
	}
	size_t erase(const key_type& key) {
		size_t idx = map.find_i(key);
		if (map.end_i() == idx) {
			return 0;
		} else {
			value_type& pval = map.val(idx);
			if (pval) {
				static_cast<Deleter&>(*this)(pval);
				pval = NULL;
			}
			map.erase_i(idx);
			return 1;
		}
	}
	size_t size() const { return map.size(); }
	bool  empty() const { return map.empty(); }

	template<class OP>
	void for_each(OP op) const { map.template for_each<OP>(op); }

	HashMap& get_map() { return map; }
	const HashMap& get_map() const { return map; }

private:
	void del_all() {
		if (map.delcnt() == 0) {
			for (size_t i = 0; i < map.end_i(); ++i) {
				value_type& p = map.val(i);
				if (p) { static_cast<Deleter&>(*this)(p); p = NULL; }
			}
		} else {
			for (size_t i = map.beg_i(); i < map.end_i(); i = map.next_i(i)) {
				value_type& p = map.val(i);
				if (p) { static_cast<Deleter&>(*this)(p); p = NULL; }
			}
		}
	}
};

#endif // __febird_fast_hash_common__  

