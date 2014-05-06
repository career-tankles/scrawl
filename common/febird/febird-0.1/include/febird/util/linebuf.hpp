#ifndef __febird_util_linebuf_hpp__
#define __febird_util_linebuf_hpp__

#include <assert.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h> // for ssize_t
#include <string.h> // strchr
#include <boost/noncopyable.hpp>

namespace febird {

struct LineBuf : boost::noncopyable {
	 size_t capacity;
	ssize_t n;
	char*   p;

	typedef char* iterator;
	typedef char* const_iterator;

	LineBuf();
	~LineBuf();

	ssize_t getline(FILE* f);

	size_t size() const { return n; }
	char* begin() const { return p; }
	char* end()   const { return p + n; }

	///@{
	///@return removed bytes
	ssize_t trim();  // remove all trailing spaces, including '\r' and '\n'
	ssize_t chomp(); // remove all trailing '\r' and '\n', just as chomp in perl
	///@}
	
	operator char*() const { return p; }

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
		char *col = p, *end = p + n;
		while (col <= end && F->size()+1 < max_fields) {
			char* next = (char*)memmem(col, end-col, delims, dlen);
			if (NULL == next) next = end;
			F->push_back(typename Vec::value_type(col, next));
			*next = 0;
			col = next + dlen;
		}
		if (col <= end)
			F->push_back(typename Vec::value_type(col, end));
		return F->size();
	}
	template<class Vec>
	size_t split_by_any(const char* delims, Vec* F, size_t max_fields = ~size_t(0)) {
		assert(n >= 0);
		size_t dlen = strlen(delims);
		if (0 == dlen) // empty delims redirect to blank delim
			return split(' ', F);
		if (1 == dlen)
			return split(delims[0], F);
		F->resize(0);
		char *col = p, *end = p + n;
		while (col <= end && F->size()+1 < max_fields) {
			char* next = col;
			while (next < end && memchr(delims, *next, dlen) == NULL) ++next;
			F->push_back(typename Vec::value_type(col, next));
			*next = 0;
			col = next + 1;
		}
		if (col <= end)
			F->push_back(typename Vec::value_type(col, end));
		return F->size();
	}
	template<class Vec>
	size_t split(const char delim, Vec* F, size_t max_fields = ~size_t(0)) {
		assert(n >= 0);
		F->resize(0);
		if (' ' == delim) {
		   	// same as awk, skip first blank field, and skip dup blanks
			char *col = p, *end = p + n;
			while (col < end && isspace(*col)) ++col; // skip first blank field
			while (col < end && F->size()+1 < max_fields) {
				char* next = col;
				while (next < end && !isspace(*next)) ++next;
				F->push_back(typename Vec::value_type(col, next));
				while (next < end &&  isspace(*next)) *next++ = 0; // skip blanks
				col = next;
			}
			if (col < end)
				F->push_back(typename Vec::value_type(col, end));
		} else {
			char *col = p, *end = p + n;
			while (col <= end && F->size()+1 < max_fields) {
				char* next = col;
				while (next < end && delim != *next) ++next;
				F->push_back(typename Vec::value_type(col, next));
				*next = 0;
				col = next + 1;
			}
			if (col <= end)
				F->push_back(typename Vec::value_type(col, end));
		}
		return F->size();
	}
};

} // namespace febird

#endif // __febird_util_linebuf_hpp__

