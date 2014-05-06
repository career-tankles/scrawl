#include "../config.hpp"
#include "profiling.hpp"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#if defined(_WIN32) || defined(_WIN64)
#  define WIN32_LEAN_AND_MEAN
#  include <Windows.h>
#else
#  include <time.h>
#  include <sys/time.h>
#endif

namespace febird {

	profiling::profiling()
	{
#if defined(_WIN32) || defined(_WIN64)
		LARGE_INTEGER li;
		QueryPerformanceFrequency(&li);
		m_freq = li.QuadPart;
#endif
	}

	long long profiling::now() const
	{
#if defined(_WIN32) || defined(_WIN64)
		LARGE_INTEGER li;
		QueryPerformanceCounter(&li);
		return li.QuadPart;
#else
		struct timespec ts;
	#if defined(__MACH__) // OS X does not have clock_gettime, use clock_get_time
		struct timeval tv;
		int ret = gettimeofday(&tv, NULL);
		if (ret != 0) {
			perror("profiling::now.gettimeofday");
			abort();
		}
		ts.tv_sec = tv.tv_sec;
		ts.tv_nsec = tv.tv_usec * 1000;
	#else
		int ret = clock_gettime(CLOCK_MONOTONIC, &ts);
		if (ret != 0) {
			perror("profiling::now.clock_gettime");
			abort();
		}
	#endif
		return (long long)ts.tv_sec * 1000000000 + ts.tv_nsec;
#endif
	}

} // namespace febird


