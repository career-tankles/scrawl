#ifndef __febird_io_file_util_hpp__
#define __febird_io_file_util_hpp__

#include <febird/io/DataIO.hpp>
#include <febird/io/FileStream.hpp>
#include <febird/io/StreamBuffer.hpp>

namespace febird {

template<class Object>
void native_load_file(const char* fname, Object* obj) {
	assert(NULL != fname);
	assert(NULL != obj);
	FileStream file(fname, "rb");
	NativeDataInput<InputBuffer> dio; dio.attach(&file);
	Object tmp;
	dio >> tmp;
	obj->swap(tmp);
}

template<class Object>
void native_save_file(const char* fname, const Object& obj) {
	assert(NULL != fname);
	FileStream file(fname, "wb");
	NativeDataOutput<OutputBuffer> dio; dio.attach(&file);
	dio << obj;
}

} // namespace febird

#endif // __febird_io_file_util_hpp__

