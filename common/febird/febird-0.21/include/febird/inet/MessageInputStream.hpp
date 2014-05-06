#ifndef __febird_inet__MessageInputStream_h__
#define __febird_inet__MessageInputStream_h__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
# pragma once
#endif

#include "../io/IStream.hpp"
#include "../io/IOException.hpp"
#include "ReactAcceptor.hpp"

namespace febird { namespace rpc {

class RequestCanceledException : public IOException
{
public:
};

struct MessageHeader
{
	uint32_t length;
	uint32_t seqid;
	uint32_t partid;

	void convert();

/**
 @brief ����˵��
  
  
 */
#define MSG_HEADER_SHIFT  30
#define MSG_HEADER_MASK  0xC0000000

#define MSG_HEADER_FIRST 1
#define MSG_HEADER_MID   0
#define MSG_HEADER_LAST  2
#define MSG_HEADER_WHOLE 3

#define MSG_HEADER_GET_TYPE(msg) ((msg) >> 30)
#define MSG_HEADER_GET_PARTID(msg) ((msg) & 0x3FFFFFFF)
};

class MessageInputStream : public IInputStream
{
	IInputStream* stream;
	Session*      session;
	MessageHeader curheader;
	size_t m_readed;

	void read_until(void* data, size_t length);

public:
	explicit MessageInputStream(IInputStream* stream);
	~MessageInputStream();

	virtual size_t read(void* vbuf, size_t length);
	virtual bool eof();

	virtual void read_hello();
};

} } // namespace febird::rpc

#endif // __febird_inet__MessageInputStream_h__
