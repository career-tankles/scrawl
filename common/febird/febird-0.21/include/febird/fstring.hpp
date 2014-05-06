#ifndef __febird_fstring_hpp__
#define __febird_fstring_hpp__

#include <assert.h>
#include <stddef.h>
#include <string.h>

#include <iterator>
#include <string>
//#include <iosfwd>
#include <utility>

#if defined(__GNUC__)
#include <stdint.h> // for uint16_t
#endif

inline size_t febird_fstrlen(const char* s) { return strlen(s); }
inline size_t febird_fstrlen(const uint16_t* s) {
	size_t n = 0;
	while (s[n]) ++n;
	return n;
}

inline char*
febird_fstrstr(const char* haystack, size_t haystack_len
			 , const char* needle  , size_t needle_len)
{ return (char*)memmem(haystack, haystack_len, needle, needle_len); }

uint16_t*
febird_fstrstr(const uint16_t* haystack, size_t haystack_len
			 , const uint16_t* needle  , size_t needle_len);

template<class Char>
struct febird_get_uchar_type;

template<>struct febird_get_uchar_type<char>{typedef unsigned char type;};
template<>struct febird_get_uchar_type<uint16_t>{typedef uint16_t type;};

// Fast String: shallow copy, simple, just has char* and length
// May be short name of: Febird String
template<class Char>
struct basic_fstring {
//	BOOST_STATIC_ASSERT(sizeof(Char) <= 2);
	typedef std::basic_string<Char> std_string;
	const Char* p;
	ptrdiff_t   n;

	basic_fstring() : p(NULL), n(0) {}
	basic_fstring(const std_string& x) : p(x.data()), n(x.size()) {}
#ifdef NDEBUG // let compiler compute strlen(string literal) at compilation time
	basic_fstring(const Char* x) : p(x), n(febird_fstrlen(x)) {}
#else
	basic_fstring(const Char* x) { assert(NULL != x); p = x; n = febird_fstrlen(x); }
#endif
	basic_fstring(const Char* x, ptrdiff_t   l) : p(x), n(l  ) { assert(l >= 0); }
	basic_fstring(const Char* x, const Char* y) : p(x), n(y-x) { assert(y >= x); }

	basic_fstring(const std::pair<Char*, Char*>& rng) : p(rng.first), n(rng.second - rng.first) { assert(n >= 0); }
	basic_fstring(const std::pair<const Char*, const Char*>& rng) : p(rng.first), n(rng.second - rng.first) { assert(n >= 0); }

	template<class CharVec>
	basic_fstring(const CharVec& chvec, typename CharVec::const_iterator** =NULL) {
		p = &chvec[0];
		n = chvec.size();
	#ifndef NDEBUG
		if (chvec.size() > 1) {
		   	assert(&chvec[0]+1 == &chvec[1]);
		   	assert(&chvec[0]+n-1 == &chvec[n-1]);
	   	}
	#endif
	}

	const std::pair<const Char*, const Char*> range() const { return std::make_pair(p, p+n); }

	typedef ptrdiff_t difference_type;
	typedef    size_t       size_type;
	typedef const Char     value_type;
	typedef const Char &reference, &const_reference;
	typedef const Char *iterator, *const_iterator;
	typedef std::reverse_iterator<iterator> reverse_iterator, const_reverse_iterator;
	typedef typename febird_get_uchar_type<Char>::type uc_t;

	iterator  begin() const { return p; }
	iterator cbegin() const { return p; }
	iterator    end() const { return p + n; }
	iterator   cend() const { return p + n; }
	reverse_iterator  rbegin() const { return reverse_iterator(p + n); }
	reverse_iterator crbegin() const { return reverse_iterator(p + n); }
	reverse_iterator    rend() const { return reverse_iterator(p); }
	reverse_iterator   crend() const { return reverse_iterator(p); }
	
	uc_t ende(ptrdiff_t off) const {
		assert(off <= n);
		assert(off >= 1);
		return p[n-off];
	}

	std_string    str() const { assert(0==n || (p && n) ); return std_string(p, n); }
	const Char* c_str() const { assert(p && '\0' == p[n]); return p; }
	const Char*  data() const { return p; }
	size_t       size() const { return n; }
	int          ilen() const { return (int)n; } // for printf: "%.*s"
	uc_t operator[](ptrdiff_t i)const{assert(i>=0);assert(i<n);assert(p);return p[i];}
	uc_t        uch(ptrdiff_t i)const{assert(i>=0);assert(i<n);assert(p);return p[i];}

	bool empty() const { return 0 == n; }

	bool begin_with(basic_fstring x) const {
		if (x.n > n) return false;
		return memcmp(p, x.p, sizeof(Char)*x.n) == 0;
	}
	bool end_with(basic_fstring x) const {
		if (x.n > n) return false;
		return memcmp(p+n - x.n, x.p, sizeof(Char)*x.n) == 0;
	}

	template<class Vec>
	size_t split(const Char delim, Vec* F, size_t max_fields = ~size_t(0)) const {
	//	assert(n >= 0);
		F->resize(0);
		if (' ' == delim) {
		   	// same as awk, skip first blank field, and skip dup blanks
			const Char *col = p, *end = p + n;
			while (col < end && isspace(*col)) ++col; // skip first blank field
			while (col < end && F->size()+1 < max_fields) {
				const Char* next = col;
				while (next < end && !isspace(*next)) ++next;
				F->push_back(typename Vec::value_type(col, next));
				while (next < end &&  isspace(*next)) ++next; // skip blanks
				col = next;
			}
			if (col < end)
				F->push_back(typename Vec::value_type(col, end));
		} else {
			const Char *col = p, *end = p + n;
			while (col <= end && F->size()+1 < max_fields) {
				const Char* next = col;
				while (next < end && delim != *next) ++next;
				F->push_back(typename Vec::value_type(col, next));
				col = next + 1;
			}
			if (col <= end)
				F->push_back(typename Vec::value_type(col, end));
		}
		return F->size();
	}

	/// split into fields
	template<class Vec>
	size_t split(const Char* delims, Vec* F, size_t max_fields = ~size_t(0)) {
		assert(n >= 0);
		size_t dlen = febird_fstrlen(delims);
		if (0 == dlen) // empty delims redirect to blank delim
			return split(' ', F);
		if (1 == dlen)
			return split(delims[0], F);
		F->resize(0);
		const Char *col = p, *end = p + n;
		while (col <= end && F->size()+1 < max_fields) {
			const Char* next = febird_fstrstr(col, end-col, delims, dlen);
			if (NULL == next) next = end;
			F->push_back(typename Vec::value_type(col, next));
			col = next + dlen;
		}
		if (col <= end)
			F->push_back(typename Vec::value_type(col, end));
		return F->size();
	}
};

template<class DataIO, class Char>
void DataIO_saveObject(DataIO& dio, basic_fstring<Char> s) {
	dio << typename DataIO::my_var_uint64_t(s.n);
	dio.ensureWrite(s.p, sizeof(Char) * s.n);
}

typedef basic_fstring<char> fstring;
typedef basic_fstring<uint16_t> fstring16;

std::string operator+(fstring x, fstring y);
inline std::string operator+(fstring x, const char* y) {return x+fstring(y);}
inline std::string operator+(const char* x, fstring y) {return fstring(x)+y;}
#if defined(__GXX_EXPERIMENTAL_CXX0X__) || __cplusplus >= 201103L
inline std::string operator+(std::string&& x, fstring y) { return x.append(   y.p, y.n); }
inline std::string operator+(fstring x, std::string&& y) { return y.insert(0, x.p, x.n); }
#endif

bool operator==(fstring x, fstring y);
bool operator!=(fstring x, fstring y);

bool operator<(fstring x, fstring y);
bool operator>(fstring x, fstring y);

bool operator<=(fstring x, fstring y);
bool operator>=(fstring x, fstring y);

/*
// ostream is brain dead binary incompatible, use inline
inline std::ostream& operator<<(std::ostream& os, fstring s) {
	os.write(s.p, s.n);
	return os;
}
*/

// fstring16
bool operator==(fstring16 x, fstring16 y);
bool operator!=(fstring16 x, fstring16 y);

bool operator<(fstring16 x, fstring16 y);
bool operator>(fstring16 x, fstring16 y);

bool operator<=(fstring16 x, fstring16 y);
bool operator>=(fstring16 x, fstring16 y);

#endif // __febird_fstring_hpp__


