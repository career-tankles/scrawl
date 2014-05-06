// strbuilder.cpp
#include "strbuilder.hpp"

#include <assert.h>
#include <errno.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#include <stdexcept>

namespace febird {
#ifdef _GNU_SOURCE
	StrPrintf::StrPrintf(const char* format, ...) {
		assert(NULL != format);
		va_list  ap;
		va_start(ap, format);
		n = vasprintf(&s, format, ap);
		va_end(ap);
		if (n < 0) {
			throw std::bad_alloc();
		}
		return;
	}
	StrPrintf::StrPrintf(std::string& dest, const char* format, ...) {
		assert(NULL != format);
		va_list  ap;
		va_start(ap, format);
		n = vasprintf(&s, format, ap);
		va_end(ap);
		if (n < 0) {
			throw std::bad_alloc();
		}
		dest.append(s, n);
	}
	StrPrintf::~StrPrintf() {
		assert(NULL != s);
		free(s);
	}
	StrPrintf::operator std::string() const {
		assert(NULL != s);
		return std::string(s, n);
	}

	// StrBuilder

	StrBuilder::StrBuilder() {
		s = NULL;
		n = 0;
		memFile = open_memstream(&s, &n);
		if (NULL == memFile) {
			throw std::runtime_error("open_memstream");
		}
	}
	StrBuilder::~StrBuilder() {
		assert(NULL != memFile);
		assert(NULL != s);
		fclose(memFile);
		free(s);
	}
	StrBuilder& StrBuilder::printf(const char* format, ...) {
		assert(NULL != format);
		assert(NULL != memFile);
		va_list ap;
		va_start(ap, format);
		int rv = vfprintf(memFile, format, ap);
		va_end(ap);
		if (rv < 0) {
			throw std::runtime_error("vfprintf on memstream");
		}
		return *this;
	}
	size_t StrBuilder::size() {
		assert(NULL != memFile);
		int rv = fflush(memFile);
		if (rv != 0) {
			perror("fflush on memstream");
			throw std::runtime_error(strerror(errno));
		}
		assert(NULL != s);
		return n;
	}
	const char* StrBuilder::c_str() {
		assert(NULL != memFile);
		int rv = fflush(memFile);
		if (rv != 0) {
			perror("fflush on memstream");
			throw std::runtime_error(strerror(errno));
		}
		assert(NULL != s);
		return s;
	}
	StrBuilder::operator std::string() const {
		assert(NULL != memFile);
		int rv = fflush(memFile);
		if (rv != 0) {
			perror("fflush on memstream");
			throw std::runtime_error(strerror(errno));
		}
		assert(NULL != s);
		return std::string(s, n);
	}
	void StrBuilder::setEof(int end_offset) {
		assert(end_offset < 0);
		assert(NULL != memFile);
		int rv = fflush(memFile);
		if (rv != 0) {
			perror("fflush on memstream");
			throw std::runtime_error(strerror(errno));
		}
		s[n+end_offset] = '\0';
		rv = fseek(memFile, end_offset, SEEK_END);
		if (rv != 0) {
			perror("fseek on memstream");
			throw std::runtime_error(strerror(errno));
		}
		assert(NULL != s);
		assert((long)n + end_offset >= 0);
	}
	void StrBuilder::setEof(int end_offset, const char* endmark) {
		assert(end_offset < 0);
		assert(NULL != endmark);
		assert(NULL != memFile);
		int rv = fseek(memFile, end_offset, SEEK_END);
		if (rv != 0) {
			perror("fflush on memstream");
			throw std::runtime_error(strerror(errno));
		}
		rv = fputs(endmark, memFile);
		if (EOF == rv) {
			perror("fputs(endmark, memFile)");
			throw std::runtime_error(strerror(errno));
		}
		rv = fflush(memFile);
		if (rv != 0) {
			perror("fflush on memstream");
			throw std::runtime_error(strerror(errno));
		}
		assert(NULL != s);
		assert((long)n + end_offset >= 0);
	}
#else
  #pragma message("strbuilder skiped because not in glibc")
#endif

} // namespace febird

