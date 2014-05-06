#ifndef __febird_util_throw_hpp__
#define __febird_util_throw_hpp__

#include "autofree.hpp"
#include <boost/current_function.hpp>

#define FEBIRD_THROW(Except, fmt, ...) \
	do { \
		febird::AutoFree<char> msg; \
		int len = asprintf(&msg.p, "%s:%d: %s: " fmt, \
			__FILE__, __LINE__, BOOST_CURRENT_FUNCTION, \
			##__VA_ARGS__); \
		fprintf(stderr, "%s\n", msg.p); \
		std::string strMsg(msg.p, len); \
		throw Except(strMsg); \
	} while (0)

#define THROW_STD(Except, fmt, ...) \
	FEBIRD_THROW(std::Except, fmt, ##__VA_ARGS__)

#endif // __febird_util_throw_hpp__

