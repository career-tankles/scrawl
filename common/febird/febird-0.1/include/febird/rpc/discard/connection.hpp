#ifndef __febird_rpc_connection_h__
#define __febird_rpc_connection_h__

#include "../io/IStream.hpp"

namespace febird { namespace rpc {

class Connection
{
	IInputStream*  is;
	IOutputStream* os;

public:
	
};


} } // namespace febird::rpc

#endif // __febird_rpc_connection_h__


