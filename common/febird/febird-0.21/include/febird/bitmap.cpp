#include "bitmap.hpp"

simplebitvec::simplebitvec(size_t bits, bool val) {
	resize(bits, val);
}
simplebitvec::simplebitvec(size_t bits, bool val, bool padding) {
	resize(bits, val);
	for (size_t i = bits, n = size(); i < n; ++i) {
		if (padding) set1(i); else set0(i);
	}
}
void simplebitvec::clear() {
	bl.clear();
}
void simplebitvec::fill(bool val) {
	bl.fill(val ? ~bm_uint_t(0) : bm_uint_t(0));
}
void simplebitvec::resize(size_t newsize, bool val) {
	bl.resize((newsize+nb-1)/nb, val ? ~bm_uint_t(0) : 0);
}
void simplebitvec::resize_no_init(size_t newsize) {
	bl.resize_no_init((newsize+nb-1)/nb);
}
void simplebitvec::resize_fill(size_t newsize, bool val) {
	resize_no_init(newsize);
	fill(val);
}

simplebitvec& simplebitvec::operator-=(const simplebitvec& y) {
	ptrdiff_t nBlocks = std::min(bl.size(), y.bl.size());
	bm_uint_t* xdata = bl.data();
	bm_uint_t const* ydata = y.bl.data();
	for (ptrdiff_t i = 0; i < nBlocks; ++i) xdata[i] &= ~ydata[i];
	return *this;
}
simplebitvec& simplebitvec::operator^=(const simplebitvec& y) {
	ptrdiff_t nBlocks = std::min(bl.size(), y.bl.size());
	bm_uint_t* xdata = bl.data();
	bm_uint_t const* ydata = y.bl.data();
	for (ptrdiff_t i = 0; i < nBlocks; ++i) xdata[i] ^= ydata[i];
	return *this;
}
simplebitvec& simplebitvec::operator&=(const simplebitvec& y) {
	ptrdiff_t nBlocks = std::min(bl.size(), y.bl.size());
	bm_uint_t* xdata = bl.data();
	bm_uint_t const* ydata = y.bl.data();
	for (ptrdiff_t i = 0; i < nBlocks; ++i) xdata[i] &= ydata[i];
	return *this;
}
simplebitvec& simplebitvec::operator|=(const simplebitvec& y) {
	ptrdiff_t nBlocks = std::min(bl.size(), y.bl.size());
	bm_uint_t* xdata = bl.data();
	bm_uint_t const* ydata = y.bl.data();
	for (ptrdiff_t i = 0; i < nBlocks; ++i) xdata[i] |= ydata[i];
	return *this;
}

void simplebitvec::block_or(const simplebitvec& y, size_t yblstart, size_t blcnt) {
	bm_uint_t* xdata = bl.data();
	bm_uint_t const* ydata = y.bl.data();
	size_t yblfinish = yblstart + blcnt;
	for (size_t i = yblstart; i < yblfinish; ++i) xdata[i] |= ydata[i];
}

void simplebitvec::block_and(const simplebitvec& y, size_t yblstart, size_t blcnt) {
	bm_uint_t* xdata = bl.data();
	bm_uint_t const* ydata = y.bl.data();
	size_t yblfinish = yblstart + blcnt;
	for (size_t i = yblstart; i < yblfinish; ++i) xdata[i] &= ydata[i];
}

bool simplebitvec::isall0() const {
	for (ptrdiff_t i = 0, n = bl.size(); i < n; ++i)
		if (bl[i])
			return false;
	return true;
}
bool simplebitvec::isall1() const {
	for (ptrdiff_t i = 0, n = bl.size(); i < n; ++i)
		if (bm_uint_t(-1) != bl[i])
			return false;
	return true;
}
size_t simplebitvec::popcnt() const {
	size_t pc = 0;
	for (ptrdiff_t i = 0, n = bl.size(); i < n; ++i)
		pc += fast_popcount(bl[i]);
	return pc;
}
size_t simplebitvec::popcnt(size_t blstart, size_t blcnt) const {
	assert(blstart <= bl.size());
	assert(blcnt <= bl.size());
	assert(blstart + blcnt <= bl.size());
	size_t pc = 0;
	for (ptrdiff_t i = blstart, n = blstart + blcnt; i < n; ++i)
		pc += fast_popcount(bl[i]);
	return pc;
}

