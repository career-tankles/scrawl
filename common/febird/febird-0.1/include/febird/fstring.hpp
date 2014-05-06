#ifndef __febird_fstring_hpp__
#define __febird_fstring_hpp__

#include <assert.h>
#include <stddef.h>
#include <string.h>

#include <iterator>
#include <string>
#include <iosfwd>
#include <utility>

// Fast String: shallow copy, simple, just has char* and length
// May be short name of: Febird String
struct fstring {
	const char* p;
	ptrdiff_t   n;

	fstring() : p(NULL), n(0) {}
	fstring(const std::string& x) : p(x.data()), n(x.size()) {}
#ifdef NDEBUG // let compiler compute strlen(string literal) at compilation time
	fstring(const char* x) : p(x), n(strlen(x)) {}
#else
	fstring(const char* x) { assert(NULL != x); p = x; n = strlen(x); }
#endif
	fstring(const char* x, ptrdiff_t   l) : p(x), n(l  ) { assert(l >= 0); }
	fstring(const char* x, const char* y) : p(x), n(y-x) { assert(y >= x); }

	fstring(const   signed char* x) { assert(NULL != x); p = reinterpret_cast<const char*>(x); n = strlen(reinterpret_cast<const char*>(x)); }
	fstring(const unsigned char* x) { assert(NULL != x); p = reinterpret_cast<const char*>(x); n = strlen(reinterpret_cast<const char*>(x)); }

	fstring(const   signed char* x, const ptrdiff_t      l) : p(reinterpret_cast<const char*>(x)), n(l  ) { assert(l >= 0); }
	fstring(const unsigned char* x, const ptrdiff_t      l) : p(reinterpret_cast<const char*>(x)), n(l  ) { assert(l >= 0); }
	fstring(const   signed char* x, const   signed char* y) : p(reinterpret_cast<const char*>(x)), n(y-x) { assert(y >= x); }
	fstring(const unsigned char* x, const unsigned char* y) : p(reinterpret_cast<const char*>(x)), n(y-x) { assert(y >= x); }

	fstring(const std::pair<char*, char*>& rng) : p(rng.first), n(rng.second - rng.first) { assert(n >= 0); }
	fstring(const std::pair<const char*, const char*>& rng) : p(rng.first), n(rng.second - rng.first) { assert(n >= 0); }
	fstring(const std::pair<signed char*, signed char*>& rng)
	   	: p(reinterpret_cast<char*>(rng.first)), n(rng.second - rng.first) { assert(n >= 0); }
	fstring(const std::pair<const signed char*, const signed char*>& rng)
	   	: p(reinterpret_cast<const char*>(rng.first)), n(rng.second - rng.first) { assert(n >= 0); }
	fstring(const std::pair<unsigned char*, unsigned char*>& rng)
	   	: p(reinterpret_cast<char*>(rng.first)), n(rng.second - rng.first) { assert(n >= 0); }

	template<class CharVec>
	fstring(const CharVec& chvec, typename CharVec::iterator** =NULL) {
		p = &chvec[0];
		n = chvec.size();
	#ifndef NDEBUG
		if (chvec.size() > 1) {
		   	assert(&chvec[0]+1 == &chvec[1]);
		   	assert(&chvec[0]+n-1 == &chvec[n-1]);
	   	}
	#endif
	}

	const std::pair<const char*, const char*> range() const { return std::make_pair(p, p+n); }

	typedef ptrdiff_t difference_type;
	typedef    size_t       size_type;
	typedef const char     value_type;
	typedef const char &reference, &const_reference;
	typedef const char *iterator, *const_iterator;
	typedef std::reverse_iterator<iterator> reverse_iterator, const_reverse_iterator;

	iterator  begin() const { return p; }
	iterator cbegin() const { return p; }
	iterator    end() const { return p + n; }
	iterator   cend() const { return p + n; }
	reverse_iterator  rbegin() const { return reverse_iterator(p + n); }
	reverse_iterator crbegin() const { return reverse_iterator(p + n); }
	reverse_iterator    rend() const { return reverse_iterator(p); }
	reverse_iterator   crend() const { return reverse_iterator(p); }
	
	char ende(ptrdiff_t off) const {
		assert(off <= n);
		assert(off >= 1);
		return p[n-off];
	}

	std::string   str() const { assert(0==n || (p && n) ); return std::string(p, n); }
	const char* c_str() const { assert(p && '\0' == p[n]); return p; }
	const char*  data() const { return p; }
	size_t       size() const { return n; }
	char operator[](ptrdiff_t i)const{assert(i>=0);assert(i<n);assert(p);return p[i];}

	bool empty() const { return 0 == n; }

	bool begin_with(fstring x) const {
		if (x.n > n) return false;
		return memcmp(p, x.p, x.n) == 0;
	}
	bool end_with(fstring x) const {
		if (x.n > n) return false;
		return memcmp(p+n - x.n, x.p, x.n) == 0;
	}

	template<class Vec>
	size_t split(const char delim, Vec* F, size_t max_fields = ~size_t(0)) const {
	//	assert(n >= 0);
		F->resize(0);
		if (' ' == delim) {
		   	// same as awk, skip first blank field, and skip dup blanks
			const char *col = p, *end = p + n;
			while (col < end && isspace(*col)) ++col; // skip first blank field
			while (col < end && F->size()+1 < max_fields) {
				const char* next = col;
				while (next < end && !isspace(*next)) ++next;
				F->push_back(typename Vec::value_type(col, next));
				while (next < end &&  isspace(*next)) ++next; // skip blanks
				col = next;
			}
			if (col < end)
				F->push_back(typename Vec::value_type(col, end));
		} else {
			const char *col = p, *end = p + n;
			while (col <= end && F->size()+1 < max_fields) {
				const char* next = col;
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
	size_t split(const char* delims, Vec* F, size_t max_fields = ~size_t(0)) {
		assert(n >= 0);
		size_t dlen = strlen(delims);
		if (0 == dlen) // empty delims redirect to blank delim
			return split(' ', F);
		if (1 == dlen)
			return split(delims[0], F);
		F->resize(0);
		const char *col = p, *end = p + n;
		while (col <= end && F->size()+1 < max_fields) {
			const char* next = (const char*)memmem(col, end-col, delims, dlen);
			if (NULL == next) next = end;
			F->push_back(typename Vec::value_type(col, next));
			col = next + dlen;
		}
		if (col <= end)
			F->push_back(typename Vec::value_type(col, end));
		return F->size();
	}
};

template<class DataIO>
void DataIO_saveObject(DataIO& dio, fstring s) {
	dio << typename DataIO::my_var_uint64_t(s.n);
	dio.ensureWrite(s.p, s.n);
}

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

std::ostream& operator<<(std::ostream& os, fstring s);

#endif // __febird_fstring_hpp__


