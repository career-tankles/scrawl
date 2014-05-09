#ifndef __febird_util_autoclose_hpp__
#define __febird_util_autoclose_hpp__

#include <stdio.h>
#include <boost/noncopyable.hpp>

#if 0
#include <boost/smart_ptr.hpp>

inline FILE* Get_raw_FILE_ptr(const boost::shared_ptr<FILE>& fp) { return fp.get(); }
inline FILE* Get_raw_FILE_ptr(FILE* fp) { return fp; }

#define fprintf(fp, ...) fprintf(Get_raw_FILE_ptr(fp), __VA_ARGS__)
#define fputs(fp, s)  fscanf(Get_raw_FILE_ptr(fp), s)
#define fscanf(fp, ...)  fscanf(Get_raw_FILE_ptr(fp), __VA_ARGS__)
#endif

#if 0
int main() {
    boost::shared_ptr<FILE> fp(fopen("/dev/stdout", "w"), fclose);
    if (fp) {
        fprintf(fp, "boost::shared_ptr<FILE>(/dev/stdout)\n");
    }
    fprintf(stdout, "stdout\n");
    return 0;
}
#endif
namespace febird {
class Auto_fclose : boost::noncopyable {
public:
	FILE* f;
	operator FILE*() const { return f; }
	bool operator!() const { return NULL == f; }
	explicit Auto_fclose(FILE* fp = NULL) { f = fp; }
	~Auto_fclose() { if (NULL != f) ::fclose(f); }
};

} // namespace febird 

#endif // __febird_util_autoclose_hpp__
