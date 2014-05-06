#include "dfa_algo.hpp"
#include "automata.hpp"
#include "dawg.hpp"
#include "linear_dfa.hpp"
#include "linear_dfa2.hpp"
#include "linear_dfa3.hpp"
#include "linear_dawg.hpp"

#include <febird/io/DataIO.hpp>
#include <febird/io/FileStream.hpp>
#include <febird/io/StreamBuffer.hpp>

DFA_Interface* DFA_Interface::load_from(fstring fname) {
	assert('\0' == *fname.end());
	febird::FileStream file(fname.p, "rb");
	return load_from(file);
}

DFA_Interface* DFA_Interface::load_from_NativeInputBuffer(void* nativeInputBufferObject) {
	using namespace febird;
	NativeDataInput<InputBuffer>& dio = *(NativeDataInput<InputBuffer>*)nativeInputBufferObject;
	std::string className;
	dio >> className;
#define ON_CLASS_IO(Class) \
		if (BOOST_STRINGIZE(Class) == className) { \
			std::auto_ptr<Class> au(new Class); \
			dio >> *au; \
			return au.release(); \
		}
#include "dfa_class_io.hpp"
	string_appender<> msg;
	msg << BOOST_CURRENT_FUNCTION << ": unknown Automata class: " << className;
	throw std::invalid_argument(msg);
}

DFA_Interface* DFA_Interface::load_from(FILE* fp) {
	using namespace febird;
	FileStream file;
	file.attach(fp);
	file.disbuf();
	std::string className;
	try {
		NativeDataInput<InputBuffer> dio; dio.attach(&file);
		dio >> className;
#define ON_CLASS_IO(Class) \
		if (BOOST_STRINGIZE(Class) == className) { \
			std::auto_ptr<Class> au(new Class); \
			dio >> *au; \
			file.detach(); \
			return au.release(); \
		}
#include "dfa_class_io.hpp"
	}
	catch (...) {
		file.detach();
		throw;
	}
	file.detach();
	string_appender<> msg;
	msg << BOOST_CURRENT_FUNCTION << ": unknown Automata class: " << className;
	throw std::invalid_argument(msg);
}

void DFA_Interface::save_to(fstring fname) const {
	assert('\0' == *fname.end());
	febird::FileStream file(fname.p, "wb");
	save_to(file);
}
void DFA_Interface::save_to(FILE* fp) const {
	using namespace febird;
	FileStream file;
	file.attach(fp);
	file.disbuf();
	try {
		NativeDataOutput<OutputBuffer> dio; dio.attach(&file);
#define ON_CLASS_IO(Class) \
		if (const Class* au = dynamic_cast<const Class*>(this)) { \
			dio << fstring(BOOST_STRINGIZE(Class)); \
			dio << *au; \
			dio.flush(); \
			file.detach(); \
			return;\
		}
#include "dfa_class_io.hpp"
	}
	catch (...) {
		file.detach();
		throw;
	}
	file.detach();
	string_appender<> msg;
	msg << BOOST_CURRENT_FUNCTION << ": unknown Automata class: " << typeid(*this).name();
	throw std::invalid_argument(msg);
}


