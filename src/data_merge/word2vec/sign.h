#ifndef _SIGN_H_
#define _SIGN_H_

#include <sys/types.h>

namespace EA_COMMON
{
	void GetSign64MD5(const char* apString,u_int32_t auiLen,u_int64_t &aui64Sign);
	void GetSign64MD5Ex(const char* apString,u_int32_t auiLen,u_int64_t &aui64Sign);
};

#endif //_SIGN_H_