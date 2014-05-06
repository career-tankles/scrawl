#include "fstring.hpp"
#include "hash_strmap.hpp"
#include <ostream>

std::string operator+(fstring x, fstring y) {
	std::string z;
	z.reserve(x.n + y.n);
	z.append(x.p, x.n);
	z.append(y.p, y.n);
	return z;
}

bool operator==(fstring x, fstring y) { return  fstring_func::equal()(x, y); }
bool operator!=(fstring x, fstring y) { return !fstring_func::equal()(x, y); }

bool operator<(fstring x, fstring y) {
	ptrdiff_t n = std::min(x.n, y.n);
	int ret = memcmp(x.p, y.p, n);
	if (ret)
		return ret < 0;
	else
		return x.n < y.n;
}
bool operator> (fstring x, fstring y) { return   y < x ; }
bool operator<=(fstring x, fstring y) { return !(y < x); }
bool operator>=(fstring x, fstring y) { return !(x < y); }

std::ostream& operator<<(std::ostream& os, fstring s) {
	os.write(s.p, s.n);
	return os;
}


