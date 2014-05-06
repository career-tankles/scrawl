/* vim: set tabstop=4 : */
#include "rpc_basic.hpp"
#include "../num_to_str.hpp"

namespace febird { namespace rpc {

void FEBIRD_DLL_EXPORT
incompitible_class_cast(remote_object* y, const char* szRequestClassName)
{
	string_appender<> oss;
	oss << "object[id=" << y->getID() << ", type=" << y->getClassName()
		<< "] is not " << szRequestClassName;
	throw rpc_exception(oss.str());
}


} } // namespace::febird::rpc
