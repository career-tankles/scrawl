#ifndef __febird_lcast_hpp_penglei__
#define __febird_lcast_hpp_penglei__

#include <string>

/// lcast_from_str must be an expiring object
class lcast_from_str {
	const char* p;
	size_t      n; // n maybe not used

	explicit lcast_from_str(const char* p1, size_t n1 = 0) : p(p1), n(n1) {}
	lcast_from_str(const lcast_from_str& y) : p(y.p), n(y.n) {}
	lcast_from_str& operator=(const lcast_from_str& y);

public:
	operator short() const;
	operator unsigned short() const;
	operator int() const;
	operator unsigned() const;
	operator long() const;
	operator unsigned long() const;
	operator long long() const;
	operator unsigned long long() const;

	operator float() const;
	operator double() const;
	operator long double() const;

	operator const std::string() const { return p; }

// non-parameter-dependent friends
friend const lcast_from_str lcast(const std::string& s);
friend const lcast_from_str lcast(const char* s, size_t n);
friend const lcast_from_str lcast(const char* s);

};

inline const lcast_from_str lcast(const std::string& s) { return lcast_from_str(s.c_str(), s.size()); }
inline const lcast_from_str lcast(const char* s, size_t n) { return lcast_from_str(s, n); }
inline const lcast_from_str lcast(const char* s) { return lcast_from_str(s); }

std::string lcast(int x);
std::string lcast(unsigned int x);
std::string lcast(short x);
std::string lcast(unsigned short x);
std::string lcast(long x);
std::string lcast(unsigned long x);
std::string lcast(long long x);
std::string lcast(unsigned long long x);
std::string lcast(float x);
std::string lcast(double x);
std::string lcast(long double x);

#endif // __febird_lcast_hpp_penglei__

