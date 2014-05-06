#include "mmap.hpp"
#include "autofree.hpp"
#include <stdio.h>

#include <stdexcept>

#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>

#if !defined(MAP_POPULATE)
	#define  MAP_POPULATE 0
#endif

namespace febird {

void mmap_close(int fd, void* base) {
	if (fd < 0)
		return;
	struct stat st;
	::fstat(fd, &st);
	::munmap(base, st.st_size);
	::close(fd);
}

void* mmap_load(const char* fname, int* fd, size_t* fsize) {
	*fd = ::open(fname, O_RDONLY);
	if (*fd < 0) {
		febird::AutoFree<char> msg;
		int len = asprintf(&msg.p, "febird_mmap_load(%s): open=%m", fname);
		fprintf(stderr, "%s\n", msg.p);
		std::string strMsg(msg.p, len);
		throw std::logic_error(strMsg);
	}
	struct stat st;
	if (::fstat(*fd, &st) < 0) {
		febird::AutoFree<char> msg;
		int len = asprintf(&msg.p, "febird_mmap_load(%s): fstat=%m", fname);
		fprintf(stderr, "%s\n", msg.p);
		::close(*fd);
		*fd = -1;
		std::string strMsg(msg.p, len);
		throw std::logic_error(strMsg);
	}
	*fsize = st.st_size;
	void* base = ::mmap(NULL, st.st_size, PROT_READ, MAP_SHARED|MAP_POPULATE, *fd, 0);
	if (MAP_FAILED == base) {
		febird::AutoFree<char> msg;
		int len = asprintf(&msg.p, "febird_mmap_load(%s): fsize=%lld mmap=%m"
				, fname, (long long)st.st_size);
		fprintf(stderr, "%s\n", msg.p);
		::close(*fd);
		*fd = -1;
		std::string strMsg(msg.p, len);
		throw std::logic_error(strMsg);
	}
	return base;
}

} // namespace febird

