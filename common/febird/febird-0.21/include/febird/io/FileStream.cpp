/* vim: set tabstop=4 : */
#include "FileStream.hpp"

#include <assert.h>
#include <string.h>

#if defined(_WIN32) || defined(WIN32) || defined(_WIN64) || defined(WIN64)
#	include <io.h>
// # if defined(_POSIX_)
// # endif
#else
#	include <unistd.h>
#endif

#	include <sys/stat.h>
#	include <sys/types.h>
#	include <fcntl.h>
#include <stdlib.h>
#	include <errno.h>
#   include "byte_io_impl.hpp"

#include "../num_to_str.hpp"

namespace febird {

FileStream::FileStream(const char* fpath, const char* mode)
{
	m_fp = NULL;
   	open(fpath, mode);
}

FileStream::FileStream(int fd, const char* mode)
{
	dopen(fd, mode);
}

FileStream::~FileStream() {
	if (m_fp)
	   	::fclose(m_fp);
}

void FileStream::ThrowOpenFileException(const char* fpath, const char* mode)
{
	string_appender<> oss;
	oss << "mode=" << mode;
	throw OpenFileException(fpath, oss.str().c_str());
}

// only can call on unopened FileStream
void FileStream::open(const char* fpath, const char* mode)
{
	assert(NULL == m_fp);
	m_fp = fopen(fpath, mode);
	if (NULL == m_fp)
		ThrowOpenFileException(fpath, mode);
}

bool FileStream::xopen(const char* fpath, const char* mode) throw()
{
	assert(NULL == m_fp);
	m_fp = fopen(fpath, mode);
	return NULL != m_fp;
}

void FileStream::dopen(int fd, const char* mode) throw()
{
	assert(NULL == m_fp);
#ifdef _MSC_VER
	m_fp = ::_fdopen(fd, mode);
#else
	m_fp = ::fdopen(fd, mode);
#endif
	if (NULL == m_fp)
	{
		char szbuf[64];
		sprintf(szbuf, "fd=%d", fd);
		ThrowOpenFileException(szbuf, mode);
	}
}

void FileStream::attach(::FILE* fp) throw()
{
	assert(NULL == m_fp);
	this->m_fp = fp;
}

FILE* FileStream::detach() throw()
{
	assert(m_fp);
	FILE* temp = m_fp;
	m_fp = NULL;
	return temp;
}

void FileStream::close() throw()
{
	assert(m_fp);
	fclose(m_fp);
	m_fp = NULL;
}

stream_position_t FileStream::tell() const
{
	assert(m_fp);
#if defined(_WIN32) || defined(WIN32) || defined(_WIN64) || defined(WIN64)
	fpos_t pos;
	if (fgetpos(m_fp, &pos) != 0)
		throw IOException(BOOST_CURRENT_FUNCTION);
	return stream_position_t(pos);
#else
	return (size_t)::ftell(m_fp);
#endif
}

void FileStream::rewind()
{
	assert(m_fp);
	::rewind(m_fp);
}

void FileStream::seek(stream_offset_t offset, int origin)
{
	assert(m_fp);
	if (::fseek(m_fp, offset, origin) != 0)
		throw IOException(BOOST_CURRENT_FUNCTION);
}

void FileStream::seek(stream_position_t pos)
{
	assert(m_fp);
#if defined(_WIN32) || defined(WIN32) || defined(_WIN64) || defined(WIN64)
	fpos_t x_fpos = pos;
	if (::fsetpos(m_fp, &x_fpos) != 0)
		throw IOException(BOOST_CURRENT_FUNCTION);
#else
	seek(long(pos), 0);
#endif
}

void FileStream::flush()
{
	assert(m_fp);
	if (::fflush(m_fp) == EOF)
		throw DelayWriteException(BOOST_CURRENT_FUNCTION);
}

stream_position_t FileStream::size() const
{
	assert(m_fp);
#if (defined(_WIN32) || defined(WIN32) || defined(_WIN64) || defined(WIN64)) && !defined(_POSIX_)
	int fno = ::_fileno(m_fp);
	return ::_filelength(fno);
#else
	struct stat x;
	if (::fstat(fileno(m_fp), &x) == 0)
		return x.st_size;
	else
		throw IOException("fstat failed");
#endif
}

size_t FileStream::read(void* buf, size_t size) throw()
{
	assert(m_fp);
	return ::fread(buf, 1, size, m_fp);
}

size_t FileStream::write(const void* buf, size_t size) throw()
{
	assert(m_fp);
	return ::fwrite(buf, 1, size, m_fp);
}

FEBIRD_GEN_ensureRead (FileStream::)
FEBIRD_GEN_ensureWrite(FileStream::)

void FileStream::disbuf() throw()
{
	assert(m_fp);
	setvbuf(m_fp, NULL, _IONBF, 0);
}

/**
 * not reentrent
 */
size_t FileStream::pread(stream_position_t pos, void* vbuf, size_t length)
{
	stream_position_t old = this->tell();
	this->seek(pos);
	size_t n = this->read(vbuf, length);
	this->seek(old);
	return n;
}

/**
 * not reentrent
 */
size_t FileStream::pwrite(stream_position_t pos, const void* vbuf, size_t length)
{
	stream_position_t old = this->tell();
	this->seek(pos);
	size_t n = this->write(vbuf, length);
	this->seek(old);
	return n;
}

NonOwnerFileStream::~NonOwnerFileStream() {
   	m_fp = NULL;
}

//////////////////////////////////////////////////////////////////////
bool FileStream::copyFile(const char* srcPath, const char* dstPath)
{
	FileStream fsrc(srcPath, "rb");
	FileStream fdst(dstPath, "wb+");

	if (fsrc && fdst)
	{
		setvbuf(fsrc.fp(), NULL, _IONBF, 0);
		setvbuf(fdst.fp(), NULL, _IONBF, 0);
		size_t nbuf = 64 * 1024;
		char*  pbuf = (char*)malloc(nbuf);
		try {
			while (!fsrc.eof())
			{
				size_t nRead  = fsrc.read(pbuf, nbuf);
				size_t nWrite = fdst.write(pbuf, nRead);
				if (nWrite != nRead) {
					throw OutOfSpaceException(BOOST_CURRENT_FUNCTION);
				}
			}
			free(pbuf);
		} catch (const std::exception& ) {
			free(pbuf);
			throw;
		}
		return true;
	}
	return false;
}


} // namespace febird

