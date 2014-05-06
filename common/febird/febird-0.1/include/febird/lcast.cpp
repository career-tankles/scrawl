#define _XOPEN_SOURCE 600
#define _ISOC99_SOURCE

#include "lcast.hpp"
#include <stdexcept>
#include <stdio.h>
#include <stdlib.h>

#ifdef __CYGWIN__
	#define strtold strtod
	#define strtof  strtod
#endif

lcast_from_str::operator short() const {
       	return atoi(p);
}

lcast_from_str::operator unsigned short() const {
	char* q;
	long l = strtoul(p, &q, 10);
	if (q == p) {
		throw std::invalid_argument("bad lcast");
	}
	return l;
}

lcast_from_str::operator int() const {
	return atoi(p);
}

lcast_from_str::operator unsigned() const {
	char* q;
	long l = strtoul(p, &q, 10);
	if (q == p) {
		throw std::invalid_argument("bad lcast");
	}
	return l;
}

lcast_from_str::operator long() const {
	char* q;
	long l = strtol(p, &q, 10);
	if (q == p) {
		throw std::invalid_argument("bad lcast");
	}
	return l;
}

lcast_from_str::operator unsigned long() const {
	char* q;
	long l = strtoul(p, &q, 10);
	if (q == p) {
		throw std::invalid_argument("bad lcast");
	}
	return l;
}

lcast_from_str::operator long long() const {
	char* q;
	long l = strtoll(p, &q, 10);
	if (q == p) {
		throw std::invalid_argument("bad lcast");
	}
	return l;
}

lcast_from_str::operator unsigned long long() const {
	char* q;
	long l = strtol(p, &q, 10);
	if (q == p) {
		throw std::invalid_argument("bad lcast");
	}
	return l;
}

lcast_from_str::operator float() const {
	char* q;
	float l = strtof(p, &q);
	if (q == p) {
		throw std::invalid_argument("bad lcast");
	}
	return l;
}

lcast_from_str::operator double() const {
	char* q;
	double l = strtod(p, &q);
	if (q == p) {
		throw std::invalid_argument("bad lcast");
	}
	return l;
}

lcast_from_str::operator long double() const {
	char* q;
	long double l = strtold(p, &q);
	if (q == p) {
		throw std::invalid_argument("bad lcast");
	}
	return l;
}

#define CONVERT(format) { \
	char buf[48]; \
	sprintf(buf, format, x); \
	return buf; \
}

std::string lcast(int x) CONVERT("%d")
std::string lcast(unsigned int x) CONVERT("%u")
std::string lcast(short x) CONVERT("%d") 
std::string lcast(unsigned short x) CONVERT("%u")
std::string lcast(long x) CONVERT("%ld")
std::string lcast(unsigned long x) CONVERT("%lu")
std::string lcast(long long x) CONVERT("%lld")
std::string lcast(unsigned long long x) CONVERT("%llu")
std::string lcast(float x) CONVERT("%f")
std::string lcast(double x) CONVERT("%f")
std::string lcast(long double x) CONVERT("%Lf")


