#include <febird/io/DataIO.hpp>
#include <febird/io/StreamBuffer.hpp>
#include "dfa_interface.hpp"

namespace febird {

class DFA_ClassMetaInfo {
public:
	// the class name in serialization data
	const char* class_name;
	std::string rtti_class_name;

	virtual ~DFA_ClassMetaInfo();

	virtual DFA_Interface* create() const = 0;
	virtual void load(NativeDataInput<InputBuffer>& dio, DFA_Interface*) const = 0;
	virtual void save(NativeDataOutput<OutputBuffer>& dio, const DFA_Interface*) const = 0;

	void register_me(const char* class_name, const char* rtti_class_name);

	static const DFA_ClassMetaInfo* find(fstring class_name);
	static const DFA_ClassMetaInfo* find(const DFA_Interface*);
};

template<class DFA>
class DFA_ClassMetaInfoInst : public DFA_ClassMetaInfo {
public:
	DFA_ClassMetaInfoInst(const char* class_name, const char* rtti_class_name) {
		this->register_me(class_name, rtti_class_name);
	}
	DFA_Interface* create() const {
		return new DFA;
	}
	void load(NativeDataInput<InputBuffer>& dio, DFA_Interface* dfa) const {
		assert(NULL != dfa);
		dio >> *static_cast<DFA*>(dfa);
	}
	void save(NativeDataOutput<OutputBuffer>& dio, const DFA_Interface* dfa) const {
		assert(NULL != dfa);
		dio << *static_cast<const DFA*>(dfa);
	}
};

#define TMPL_INST_DFA_CLASS(DFA) \
	static DFA_ClassMetaInfoInst<DFA >  \
		BOOST_PP_CAT(gs_dfa_tmplinst, __LINE__)(#DFA, typeid(DFA).name());

} // namespace febird

