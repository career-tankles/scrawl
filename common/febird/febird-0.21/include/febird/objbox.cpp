#include "objbox.hpp"

namespace febird { namespace objbox {

void intrusive_ptr_release(obj* p) {
	assert(p->refcnt > 0);
	if (0 == --p->refcnt)
		delete p;
}

obj::~obj() {}

obj_ptr::obj_ptr(const char* str)
	: super(new obj_string(str)) {}

FEBIRD_BOXING_OBJECT_IMPL(obj_bool   ,   bool)
FEBIRD_BOXING_OBJECT_IMPL(obj_short  ,   signed short)
FEBIRD_BOXING_OBJECT_IMPL(obj_ushort , unsigned short)
FEBIRD_BOXING_OBJECT_IMPL(obj_int    ,   signed int)
FEBIRD_BOXING_OBJECT_IMPL(obj_uint   , unsigned int)
FEBIRD_BOXING_OBJECT_IMPL(obj_long   ,   signed long)
FEBIRD_BOXING_OBJECT_IMPL(obj_ulong  , unsigned long)
FEBIRD_BOXING_OBJECT_IMPL(obj_llong  ,   signed long long)
FEBIRD_BOXING_OBJECT_IMPL(obj_ullong , unsigned long long)
FEBIRD_BOXING_OBJECT_IMPL(obj_float  , float)
FEBIRD_BOXING_OBJECT_IMPL(obj_double , double)
FEBIRD_BOXING_OBJECT_IMPL(obj_ldouble, long double)

FEBIRD_BOXING_OBJECT_DERIVE_IMPL(obj_string , std::string)
FEBIRD_BOXING_OBJECT_DERIVE_IMPL(obj_array  , valvec<obj_ptr >)

} } // febird::objbox

