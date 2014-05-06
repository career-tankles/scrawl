#include "linebuf.hpp"

#include <assert.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <stdexcept>

namespace febird {

LineBuf::LineBuf()
   	: capacity(0), n(0), p(NULL)
{}

LineBuf::~LineBuf() {
   	if (p)
	   	free(p);
}

ssize_t LineBuf::getline(FILE* f) {
	assert(NULL != f);
#if defined(__USE_GNU) // has ::getline
	return n = ::getline(&p, &capacity, f);
#else
//	#error only _GNU_SOURCE is supported
	if (NULL == p) {
		capacity = BUFSIZ;
		p = (char*)malloc(BUFSIZ);
	}
	char* ret = ::fgets(p, capacity, f);
	if (ret) {
		size_t len = ::strlen(p);
		while (len == capacity-1 && p[len-1] == '\n') {
			ret = (char*)realloc(p, capacity*2);
			if (NULL == ret) {
				throw std::bad_alloc();
			}
			p = ret;
			ret = ::fgets(p + len, capacity+1, f);
			len = len + ::strlen(ret);
			capacity *= 2;
		}
		return n = ssize_t(len);
	} else {
		return -1;
	}
#endif
}

ssize_t LineBuf::trim() {
	assert(NULL != p);
	ssize_t n0 = n;
	while (n > 0 && isspace(p[n-1])) p[--n] = 0;
	return n0 - n;
}

ssize_t LineBuf::chomp() {
	assert(NULL != p);
	ssize_t n0 = n;
	while (n > 0 && strchr("\r\n", p[n-1])) p[--n] = 0;
	return n0 - n;
}

} // namespace febird

