#ifndef __febird_util_mmap_hpp__
#define __febird_util_mmap_hpp__

#include <stddef.h>

namespace febird {

void  mmap_close(int fd, void* base);
void* mmap_load(const char* fname, int* fd, size_t* base);

} // namespace febird 

#endif // __febird_util_mmap_hpp__

