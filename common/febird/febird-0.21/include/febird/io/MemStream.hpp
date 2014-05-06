/* vim: set tabstop=4 : */
#ifndef __febird_io_AutoGrownMemIO_h__
#define __febird_io_AutoGrownMemIO_h__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
# pragma once
#endif

#include <assert.h>
#include <string.h> // for memcpy
#include <stdarg.h>
#include <stdio.h>  // for size_t, FILE
#include <stdexcept>
#include <utility>
#include <boost/current_function.hpp>
//#include <boost/type_traits/integral_constant.hpp>
#include <boost/mpl/bool.hpp>

#include <febird/util/throw.hpp>
#include "../stdtypes.hpp"
#include "IOException.hpp"
#include "var_int.hpp"

namespace febird {

FEBIRD_DLL_EXPORT void throw_EndOfFile (const char* func, size_t want, size_t available) febird_no_return;
FEBIRD_DLL_EXPORT void throw_OutOfSpace(const char* func, size_t want, size_t available) febird_no_return;

//! MinMemIO
//! +--MemIO
//!    +--SeekableMemIO
//!       +--AutoGrownMemIO

/**
 @brief 最有效的MemIO
 
 只用一个指针保存当前位置，没有范围检查，只应该在完全可预测的情况下使用这个类

 @note
  -# 如果无法预测是否会越界，禁止使用该类
 */
class FEBIRD_DLL_EXPORT MinMemIO
{
public:
	typedef boost::mpl::false_ is_seekable; //!< 不能 seek

	explicit MinMemIO(void* buf = 0) : m_pos((unsigned char*)buf) {}

	void set(void* vptr) { m_pos = (unsigned char*)vptr; }
	void set(MinMemIO y) { m_pos = y.m_pos; }

	byte readByte() { return *m_pos++; }
	int  getByte() { return *m_pos++; }
	void writeByte(byte b) { *m_pos++ = b; }

	void ensureRead(void* data, size_t length) {
		memcpy(data, m_pos, length);
		m_pos += length;
	}
	void ensureWrite(const void* data, size_t length) {
		memcpy(m_pos, data, length);
		m_pos += length;
	}

	size_t read(void* data, size_t length) {
		memcpy(data, m_pos, length);
		m_pos += length;
		return length;
	}
	size_t write(const void* data, size_t length) {
		memcpy(m_pos, data, length);
		m_pos += length;
		return length;
	}

	void flush() {} // do nothing...

	byte* current() const { return m_pos; }

	//! caller can use this function to determine an offset difference
	ptrdiff_t diff(const void* start) const throw() { return m_pos - (byte*)start; }

	void skip(ptrdiff_t diff) throw() {	m_pos += diff; }

	byte uncheckedReadByte() { return *m_pos++; }
	void uncheckedWriteByte(byte b) { *m_pos++ = b; }

	template<class InputStream>
	void from_input(InputStream& input, size_t length) {
		input.ensureRead(m_pos, length);
		m_pos += length;
	}
	template<class OutputStream>
	void to_output(OutputStream& output, size_t length) {
		output.ensureWrite(m_pos, length);
		m_pos += length;
	}
	
	ptrdiff_t buf_remain_bytes() const { return INT_MAX; }

	FEBIRD_DECL_FAST_VAR_INT_READER()
	FEBIRD_DECL_FAST_VAR_INT_WRITER()

protected:
	byte* m_pos;
};

/**
 @brief Mem Stream 操作所需的最小集合
  
  这个类的尺寸非常小，在极端情况下效率非常高，在使用外部提供的缓冲时，这个类是最佳的选择
  这个类可以安全地浅拷贝
 */
class FEBIRD_DLL_EXPORT MemIO : public MinMemIO
{
public:
	MemIO() { m_end = NULL; }
	MemIO(void* buf, size_t size) { set(buf, size); }
	MemIO(void* beg, void* end) { set(beg, end); }

	void set(void* buf, size_t size) {
		m_pos = (byte*)buf;
		m_end = (byte*)buf + size;
	}
	void set(void* beg, void* end) {
		m_pos = (byte*)beg;
		m_end = (byte*)end;
	}
	void set(MemIO y) { *this = y; }

	//! test pos reach end or not
	bool eof() const throw() {
		assert(m_pos <= m_end);
		return m_pos == m_end;
	}

	byte readByte() throw(EndOfFileException);
	int  getByte() throw();
	void writeByte(byte b) throw(OutOfSpaceException);

	void ensureRead(void* data, size_t length) ;
	void ensureWrite(const void* data, size_t length);

	size_t read(void* data, size_t length) throw();
	size_t write(const void* data, size_t length) throw();

	// rarely used methods....
	//
	size_t remain() const throw() { return m_end - m_pos; }
	byte*  end() const throw() { return m_end; }

	/**
	 @brief 向前跳过 @a diff 个字节
	 @a 可以是负数，表示向后跳跃
	 */
	void skip(ptrdiff_t diff) {
		assert(m_pos + diff <= m_end);
		if (febird_likely(m_pos + diff <= m_end))
			m_pos += diff;
		else {
			THROW_STD(invalid_argument
				, "diff=%ld is too large, end-pos=%ld"
				, long(diff), long(m_end-m_pos));
		}
	}
	ptrdiff_t buf_remain_bytes() const { return m_end - m_pos; }

	template<class InputStream>
	void from_input(InputStream& input, size_t length){
		if (febird_unlikely(m_pos + length > m_end))
			throw_OutOfSpace(BOOST_CURRENT_FUNCTION, length);
		input.ensureRead(m_pos, length);
		m_pos += length;
	}
	template<class OutputStream>
	void to_output(OutputStream& output, size_t length){
		if (febird_unlikely(m_pos + length > m_end))
			throw_EndOfFile(BOOST_CURRENT_FUNCTION, length);
		output.ensureWrite(m_pos, length);
		m_pos += length;
	}

	FILE* forInputFILE();

	FEBIRD_DECL_FAST_VAR_INT_READER()
	FEBIRD_DECL_FAST_VAR_INT_WRITER()

protected:
	void throw_EndOfFile(const char* func, size_t want);
	void throw_OutOfSpace(const char* func, size_t want);

protected:
	byte* m_end; // only used by set/eof
};

class FEBIRD_DLL_EXPORT AutoGrownMemIO;

class FEBIRD_DLL_EXPORT SeekableMemIO : public MemIO
{
public:
	typedef boost::mpl::true_ is_seekable; //!< 可以 seek

	SeekableMemIO() { m_pos = m_beg = m_end = 0; }
	SeekableMemIO(void* buf, size_t size) { set(buf, size); }
	SeekableMemIO(void* beg, void* end) { set(beg, end); }
	SeekableMemIO(const MemIO& x) { set(x.current(), x.end()); }

	void set(void* buf, size_t size) throw() {
		m_pos = (byte*)buf;
		m_beg = (byte*)buf;
		m_end = (byte*)buf + size;
	}
	void set(void* beg, void* end) throw() {
		m_pos = (byte*)beg;
		m_beg = (byte*)beg;
		m_end = (byte*)end;
	}

	byte*  begin()const throw() { return m_beg; }
	byte*  buf()  const throw() { return m_beg; }
	size_t size() const throw() { return m_end-m_beg; }

	size_t tell() const throw() { return m_pos-m_beg; }

	void rewind() throw() { m_pos = m_beg; }
	void seek(ptrdiff_t newPos);
	void seek(ptrdiff_t offset, int origin);

	void swap(SeekableMemIO& that) {
		std::swap(m_beg, that.m_beg);
		std::swap(m_end, that.m_end);
		std::swap(m_pos, that.m_pos);
	}

	//@{
	//! return part of (*this) as a MemIO
	MemIO range(size_t ibeg, size_t iend) const;
	MemIO head() const throw() { return MemIO(m_beg, m_pos); }
	MemIO tail() const throw() { return MemIO(m_pos, m_end); }
	MemIO whole()const throw() { return MemIO(m_beg, m_end); }
	//@}

protected:
	byte* m_beg;

private:
	SeekableMemIO(AutoGrownMemIO&);
	SeekableMemIO(const AutoGrownMemIO&);
};

/**
 @brief AutoGrownMemIO 可以管理自己的 buffer

 @note
  - 如果只需要 Eofmark, 使用 MemIO 就可以了
  - 如果还需要 seekable, 使用 SeekableMemIO
 */
//template<bool Use_c_malloc>
class FEBIRD_DLL_EXPORT AutoGrownMemIO : public SeekableMemIO
{
	DECLARE_NONE_COPYABLE_CLASS(AutoGrownMemIO);

	void growAndWrite(const void* data, size_t length);
	void growAndWriteByte(byte b);

public:
	explicit AutoGrownMemIO(size_t size = 0);

	~AutoGrownMemIO();

	void writeByte(byte b) {
		assert(m_pos <= m_end);
		if (febird_likely(m_pos < m_end))
			*m_pos++ = b;
		else
			growAndWriteByte(b);
	}

	void ensureWrite(const void* data, size_t length) {
		assert(m_pos <= m_end);
		if (febird_likely(m_pos + length <= m_end)) {
			memcpy(m_pos, data, length);
			m_pos += length;
		} else
			growAndWrite(data, length);
	}

	size_t write(const void* data, size_t length) throw() {
		ensureWrite(data, length);
		return length;
	}

	size_t printf(const char* format, ...)
#ifdef __GNUC__
	__attribute__ ((__format__ (__printf__, 2, 3)))
#endif
	;

	size_t vprintf(const char* format, va_list ap)
#ifdef __GNUC__
	__attribute__ ((__format__ (__printf__, 2, 0)))
#endif
	;

	FILE* forFILE(const char* mode);
	void clone(const AutoGrownMemIO& src);

	// rarely used methods....
	//
	void resize(size_t newsize);
	void init(size_t size);

	template<class InputStream>
	void from_input(InputStream& input, size_t length) {
		if (febird_unlikely(m_pos + length > m_end))
			resize(tell() + length);
		input.ensureRead(m_pos, length);
		m_pos += length;
	}

	void swap(AutoGrownMemIO& that) { SeekableMemIO::swap(that); }

	template<class DataIO>
	friend
	void DataIO_loadObject(DataIO& dio, AutoGrownMemIO& x) {
		var_size_t length;
		dio >> length;
		x.resize(length.t);
		dio.ensureRead(x.begin(), length.t);
	}

	template<class DataIO>
	friend
	void DataIO_saveObject(DataIO& dio, const AutoGrownMemIO& x) {
		dio << var_size_t(x.tell());
		dio.ensureWrite(x.begin(), x.tell());
	}

	FEBIRD_DECL_FAST_VAR_INT_WRITER()

private:
	//@{
	//! disable MemIO::set
	//!
	void set(void* buf, size_t size);
	void set(void* beg, void* end);
	//@}

	//@{
	//! disable convert-ability to MemIO
	//! this cause gcc warning: conversion to a reference to a base class will never use a type conversion operator
	//! see SeekableMemIO::SeekableMemIO(const AutoGrownMemIO&)
//	operator const SeekableMemIO&() const;
//	operator SeekableMemIO&();
	//@}
};

//////////////////////////////////////////////////////////////////////////

/**
 * @brief 读取 length 长的数据到 data
 * 
 * 这个函数还是值得 inline 的，可以参考如下手工的汇编代码：
 *
 * inlined in caller, 省略了寄存器保存和恢复指令，实际情况下也有可能不用保存和恢复
 *   mov eax, m_end
 *   sub eax, m_pos
 *   mov ecx, length
 *   mov esi, m_pos
 *   mov edi, data
 *   cld
 *   cmp eax, ecx
 *   jl  Overflow
 *   rep movsb
 *   jmp End
 * Overflow:
 *   mov ecx, eax
 *   rep movsb
 * End:
 *   mov m_pos, esi
 * --------------------------------
 * sub routine in caller:
 *   push length
 *   push data
 *   push this
 *   call MemIO::read
 *   add  esp, 12 ; 如果是 stdcall, 则没有这条语句
 */
inline size_t MemIO::read(void* data, size_t length) throw()
{
	register ptrdiff_t n = m_end - m_pos;
	if (febird_unlikely(n < ptrdiff_t(length))) {
		memcpy(data, m_pos, n);
	//	m_pos = m_end;
		m_pos += n;
		return n;
	} else {
		memcpy(data, m_pos, length);
		m_pos += length;
		return length;
	}
}

inline size_t MemIO::write(const void* data, size_t length) throw()
{
	register ptrdiff_t n = m_end - m_pos;
	if (febird_unlikely(n < ptrdiff_t(length))) {
		memcpy(m_pos, data, n);
	//	m_pos = m_end;
		m_pos += n;
		return n;
	} else {
		memcpy(m_pos, data, length);
		m_pos += length;
		return length;
	}
}

inline void MemIO::ensureRead(void* data, size_t length)
{
	if (febird_likely(m_pos + length <= m_end)) {
		memcpy(data, m_pos, length);
		m_pos += length;
	} else
		throw_EndOfFile(BOOST_CURRENT_FUNCTION, length);
}

inline void MemIO::ensureWrite(const void* data, size_t length)
{
	if (febird_likely(m_pos + length <= m_end)) {
		memcpy(m_pos, data, length);
		m_pos += length;
	} else
		throw_OutOfSpace(BOOST_CURRENT_FUNCTION, length);
}

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4715) // not all control paths return a value
#endif

inline byte MemIO::readByte() throw(EndOfFileException)
{
	if (febird_likely(m_pos < m_end))
		return *m_pos++;
	else {
		throw_EndOfFile(BOOST_CURRENT_FUNCTION, 1);
		return 0; // remove compiler warning
	}
}

#ifdef _MSC_VER
#pragma warning(pop)
#endif

inline void MemIO::writeByte(byte b) throw(OutOfSpaceException)
{
	if (febird_likely(m_pos < m_end))
		*m_pos++ = b;
	else
		throw_OutOfSpace(BOOST_CURRENT_FUNCTION, 1);
}
inline int MemIO::getByte() throw()
{
	if (febird_likely(m_pos < m_end))
		return *m_pos++;
	else
		return -1;
}

//////////////////////////////////////////////////////////////////////////

// AutoGrownMemIO can be dumped into DataIO

//////////////////////////////////////////////////////////////////////////

} // namespace febird

#endif

