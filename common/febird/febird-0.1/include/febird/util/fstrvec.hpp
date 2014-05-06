#ifndef __febird_fstrvec_hpp__
#define __febird_fstrvec_hpp__

#include <febird/valvec.hpp>
#include <string>
#include <utility>


// 'Offset' could be a struct which contains offset as a field
template<class Offset>
struct default_offset_op {
	size_t get(const Offset& x) const { return x; }
	void   set(Offset& x, size_t y) const { x = static_cast<Offset>(y); }
	void   inc(Offset& x, ptrdiff_t d = 1) const { x += d; }
	Offset make(size_t y) const { return Offset(y); }
};

// just allow operations at back
//
template< class Char
		, class Offset = unsigned
		, class OffsetOp = default_offset_op<Offset>
		>
class basic_fstrvec : private OffsetOp {
	template<class> struct void_ { typedef void type; };
public:
	valvec<Char>   strpool;
	valvec<Offset> offsets;
	const size_t   maxpool = Offset(~Offset(0));

	explicit basic_fstrvec(const OffsetOp& oop = OffsetOp()) : OffsetOp(oop) {
		offsets.push_back(OffsetOp::make(0));
	}

	void reserve(size_t capacity) {
		offsets.reserve(capacity+1);
	}
	void reserve_strpool(size_t capacity) {
		strpool.reserve(capacity);
	}

	template<class String>
	typename void_<typename String::iterator>::type
   	push_back(const String& str) {
		assert(strpool.size() + str.size() <= maxpool);
		strpool.append(str.begin(), str.end());
		offsets.push_back(OffsetOp::make(strpool.size()));
	}
	void push_back(std::pair<const Char*, const Char*> range) {
		assert(range.first <= range.second);
		assert(strpool.size() + (range.second - range.first) <= maxpool);
		strpool.append(range.first, range.second);
		offsets.push_back(OffsetOp::make(strpool.size()));
	}
	void emplace_back(const Char* str, size_t len) {
		assert(strpool.size() + len <= maxpool);
		strpool.append(str, len);
		offsets.push_back(OffsetOp::make(strpool.size()));
	}
	template<class InputIterator>
	void emplace_back(InputIterator first, InputIterator last) {
		strpool.append(first, last);
		offsets.push_back(OffsetOp::make(strpool.size()));
	}

	void back_append(std::pair<const Char*, const Char*> range) {
		assert(range.first <= range.second);
		assert(strpool.size() + (range.second - range.first) <= maxpool);
		assert(offsets.size() >= 2);
		strpool.append(range.first, range.second);
		OffsetOp::inc(offsets.back(), range.second - range.first);
	}
	template<class InputIterator>
	void back_append(InputIterator first, InputIterator last) {
		assert(offsets.size() >= 2);
		strpool.append(first, last);
		OffsetOp::inc(offsets.back(), last - first);
	}
	void back_append(const Char* str, size_t len) {
		assert(strpool.size() + len <= maxpool);
		assert(offsets.size() >= 2);
		strpool.append(str, len);
		OffsetOp::inc(offsets.back(), len);
	}
	void back_append(Char ch) {
		assert(strpool.size() + 1 <= maxpool);
		assert(offsets.size() >= 2);
		strpool.push_back(ch);
		OffsetOp::inc(offsets.back(), 1);
	}

	void pop_back() {
		assert(offsets.size() >= 2);
		offsets.pop_back();
		strpool.resize(OffsetOp::get(offsets.back()));
	}

	void resize(size_t n) {
		assert(n < offsets.size() && "basic_fstrvec::resize just allow shrink");
		if (n >= offsets.size()) {
		   	throw std::logic_error("basic_fstrvec::resize just allow shrink");
		}
		offsets.resize(n+1);
		strpool.resize(OffsetOp::get(offsets.back()));
	}

	std::pair<const Char*, const Char*> front() const {
	   	assert(offsets.size() >= 2);
		return (*this)[0];
   	}
	std::pair<const Char*, const Char*> back() const {
	   	assert(offsets.size() >= 2);
	   	return (*this)[offsets.size()-2];
	}

	size_t size() const { return offsets.size() - 1; }
	bool  empty() const { return strpool.empty(); }

	std::pair<Char*, Char*> operator[](size_t idx) {
		assert(idx < offsets.size()-1);
		Char* base = strpool.data();
		size_t off0 = OffsetOp::get(offsets[idx+0]);
		size_t off1 = OffsetOp::get(offsets[idx+1]);
		assert(off0 <= off1);
		return std::pair<Char*, Char*>(base + off0, base + off1);
	}
	std::pair<const Char*, const Char*> operator[](size_t idx) const {
		assert(idx < offsets.size()-1);
		const Char* base = strpool.data();
		size_t off0 = OffsetOp::get(offsets[idx+0]);
		size_t off1 = OffsetOp::get(offsets[idx+1]);
		assert(off0 <= off1);
		return std::pair<const Char*, const Char*>(base + off0, base + off1);
	}
	std::pair<Char*, Char*> at(size_t idx) {
		if (idx >= offsets.size()-1) {
			throw std::out_of_range("basic_fstrvec: at");
		}
		return (*this)[idx];
	}
	std::pair<const Char*, const Char*> at(size_t idx) const {
		if (idx >= offsets.size()-1) {
			throw std::out_of_range("basic_fstrvec: at");
		}
		return (*this)[idx];
	}
	std::basic_string<Char> str(size_t idx) const {
		assert(idx < offsets.size()-1);
		std::pair<const Char*, const Char*> x = (*this)[idx];
		return std::basic_string<Char>(x.first, x.second);
	}

	size_t slen(size_t idx) const {
		assert(idx < offsets.size()-1);
		size_t off0 = OffsetOp::get(offsets[idx+0]);
		size_t off1 = OffsetOp::get(offsets[idx+1]);
		assert(off0 <= off1);
		return off1 - off0;
	}

	Char* beg_of(size_t idx) {
		assert(idx < offsets.size()-1);
		Char* base = strpool.data();
		size_t off0 = OffsetOp::get(offsets[idx]);
		assert(off0 <= strpool.size());
		return base + off0;
	}
	Char* beg_of(size_t idx) const {
		assert(idx < offsets.size()-1);
		const Char* base = strpool.data();
		size_t off0 = OffsetOp::get(offsets[idx]);
		assert(off0 <= strpool.size());
		return base + off0;
	}
	Char* end_of(size_t idx) {
		assert(idx < offsets.size()-1);
		Char* base = strpool.data();
		size_t off1 = OffsetOp::get(offsets[idx+1]);
		assert(off1 <= strpool.size());
		return base + off1;
	}
	const Char* end_of(size_t idx) const {
		assert(idx < offsets.size()-1);
		const Char* base = strpool.data();
		size_t off1 = OffsetOp::get(offsets[idx+1]);
		assert(off1 <= strpool.size());
		return base + off1;
	}

	void shrink_to_fit() {
		strpool.shrink_to_fit();
		offsets.shrink_to_fit();
	}

	void swap(basic_fstrvec& y) {
		std::swap(static_cast<OffsetOp&>(*this), static_cast<OffsetOp&>(y));
		strpool.swap(y.strpool);
		offsets.swap(y.offsets);
	}

	template<class DataIO>
	friend void DataIO_saveObject(DataIO& dio, const basic_fstrvec& x) {
		dio << x.strpool;
		dio << x.offsets;
	}
	template<class DataIO>
	friend void DataIO_loadObject(DataIO& dio, basic_fstrvec& x) {
		dio >> x.strpool;
		dio >> x.offsets;
	}
};

typedef basic_fstrvec<char, unsigned int > fstrvec;
typedef basic_fstrvec<char, unsigned long> fstrvecl;

typedef basic_fstrvec<wchar_t, unsigned int > wfstrvec;
typedef basic_fstrvec<wchar_t, unsigned long> wfstrvecl;

namespace std {
	template<class Char, class Offset>
	void swap(basic_fstrvec<Char, Offset>& x, basic_fstrvec<Char, Offset>& y)
   	{ x.swap(y); }
}


#endif // __febird_fstrvec_hpp__

