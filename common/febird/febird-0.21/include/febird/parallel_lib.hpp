#ifndef __febird_parallel_lib_hpp__
#define __febird_parallel_lib_hpp__

#if defined(FEBIRD_ENABLE_PARALLEL) && defined(__GNUC__) && __GNUC__ * 1000 + __GNUC_MINOR__ >= 4007
	#include <parallel/algorithm>
	#define febird_parallel_sort __gnu_parallel::sort
#else
	#include <algorithm>
	#define febird_parallel_sort std::sort
#endif

#endif // __febird_parallel_lib_hpp__

