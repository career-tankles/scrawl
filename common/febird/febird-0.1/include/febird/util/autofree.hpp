#ifndef __febird_util_autofree_hpp__
#define __febird_util_autofree_hpp__

#include <boost/noncopyable.hpp>
#include <boost/static_assert.hpp>
#include <boost/type_traits.hpp>

namespace febird {

	template<class T>
	class AutoFree : boost::noncopyable {
		BOOST_STATIC_ASSERT(boost::has_trivial_destructor<T>::value);

	public:
		T* p; // just the pointer, no any overhead

		explicit AutoFree(T* p = 0) : p(p) {}
		~AutoFree() { if (p) ::free(p); }

		void operator=(T* q) {
		   	if (p)
			   	::free(p);
		   	p = q;
	   	}

		void free() {
		   	assert(p);
		   	::free(p);
		   	p = NULL;
	   	}

		operator T*  () const { return  p; }
		T* operator->() const { return  p; } // ? direct, simple and stupid ?
		T& operator* () const { return *p; } // ? direct, simple and stupid ?
	//	T& operator[](int i) const { return p[i]; }
	};

} // namespace febird

#endif


