#include "tmplinst.hpp"
#include <febird/hash_strmap.hpp>

namespace febird {

static hash_strmap<const DFA_ClassMetaInfo*>& gs_by_dio_class_name() {
	static hash_strmap<const DFA_ClassMetaInfo*> me;
	return me;
}
static hash_strmap<const DFA_ClassMetaInfo*>& gs_by_rtti_class_name () {
	static hash_strmap<const DFA_ClassMetaInfo*> me;
	return me;
}

DFA_ClassMetaInfo::~DFA_ClassMetaInfo() {
}

const DFA_ClassMetaInfo* DFA_ClassMetaInfo::find(fstring class_name) {
	size_t idx = gs_by_dio_class_name().find_i(class_name);
	if (gs_by_dio_class_name().end_i() != idx)
		return gs_by_dio_class_name().val(idx);
	else
		return NULL;
}

const DFA_ClassMetaInfo* DFA_ClassMetaInfo::find(const DFA_Interface* dfa) {
	size_t idx = gs_by_rtti_class_name().find_i(typeid(*dfa).name());
	if (gs_by_rtti_class_name().end_i() != idx)
		return gs_by_rtti_class_name().val(idx);
	else
		return NULL;
}

void DFA_ClassMetaInfo::register_me(const char* class_name, const char* rtti_class_name) {
//	fprintf(stderr, "register_class: %s\n", meta->class_name);
	this->class_name = class_name;
	this->rtti_class_name.assign(rtti_class_name);
	gs_by_dio_class_name().insert_i(class_name, this);
	gs_by_rtti_class_name().insert_i(rtti_class_name, this);
}

} // namespace febird

