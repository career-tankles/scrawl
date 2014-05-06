#ifndef __febird_objbox_hpp__
#define __febird_objbox_hpp__

#include <febird/valvec.hpp>

#include <stddef.h>
#include <string>
#include <boost/smart_ptr/intrusive_ptr.hpp>
#include <boost/noncopyable.hpp>
#include <boost/utility/enable_if.hpp>

namespace febird { namespace objbox {

	template<class> struct obj_val; // map boxed type to boxer type

	class obj : boost::noncopyable {
	public:
		ptrdiff_t refcnt;
		friend void intrusive_ptr_add_ref(obj* p) { ++p->refcnt; }
		friend void intrusive_ptr_release(obj* p);
		obj() { refcnt = 0; }
		virtual ~obj();
	};

	class obj_ptr : public boost::intrusive_ptr<obj> {
		typedef boost::intrusive_ptr<obj> super;
	public:
		explicit obj_ptr(obj* p = NULL) : super(p) {}
		explicit obj_ptr(const char* p);

		template<class T>
		explicit obj_ptr(T* p,
				typename boost::enable_if<boost::is_base_of<obj,T> >::type* = NULL)
	   	 : super(p) {}

		template<class T>
		explicit obj_ptr(const boost::intrusive_ptr<T>& p,
				typename boost::enable_if<boost::is_base_of<obj,T> >::type* = NULL)
		 : super(p) {}

		template<class T>
		explicit obj_ptr(const T& x, typename obj_val<T>::type* = NULL)
	   	{ this->reset(new typename obj_val<T>::type(x)); }

		template<class T>
		typename boost::enable_if
			< boost::is_base_of< obj
							   , typename obj_val<T>::type
							   >
			, obj_ptr
			>::type&
		operator=(const T& x) { this->reset(new typename obj_val<T>::type(x)); }
		obj_ptr& operator=(obj* x) { super::reset(x); return *this; }
//		using super::operator=;
	};

	template<class T> const T& obj_cast(const obj_ptr& p) {
		if (NULL == p.get())
			throw std::logic_error("obj_cast: dereferencing NULL pointer");
		return dynamic_cast<const typename obj_val<T>::type&>(*p).get_boxed();
	}
	template<class T> T& obj_cast(obj_ptr& p) {
		if (NULL == p.get())
			throw std::logic_error("obj_cast: dereferencing NULL pointer");
		return dynamic_cast<typename obj_val<T>::type&>(*p).get_boxed();
	}
	template<class T> T& obj_cast(obj* p) {
		if (NULL == p)
			throw std::logic_error("obj_cast: dereferencing NULL pointer");
		return dynamic_cast<typename obj_val<T>::type&>(*p).get_boxed();
	}
	template<class T> const T& obj_cast(const obj* p) {
		if (NULL == p)
			throw std::logic_error("obj_cast: dereferencing NULL pointer");
		return dynamic_cast<const typename obj_val<T>::type&>(*p).get_boxed();
	}
	template<class T> T& obj_cast(obj& p) {
		if (NULL == &p)
			throw std::logic_error("obj_cast: dereferencing NULL reference");
		return dynamic_cast<typename obj_val<T>::type&>(p).get_boxed();
	}
	template<class T> const T& obj_cast(const obj& p) {
		if (NULL == &p)
			throw std::logic_error("obj_cast: dereferencing NULL reference");
		return dynamic_cast<const typename obj_val<T>::type&>(p).get_boxed();
	}

// Boxer::get_boxed() is non-virtual
#define FEBIRD_BOXING_OBJECT(Boxer, Boxed) \
	class Boxer : public obj { public: \
		Boxed t; \
		Boxer(); \
		explicit Boxer(const Boxed& y); \
		~Boxer(); \
		const Boxed& get_boxed() const { return t; } \
		      Boxed& get_boxed()       { return t; } \
	}; \
	template<> struct obj_val<Boxed> { typedef Boxer type; };
//-----------------------------------------------------------------
#define FEBIRD_BOXING_OBJECT_IMPL(Boxer, Boxed) \
	Boxer::Boxer() {} \
	Boxer::Boxer(const Boxed& y) : t(y) {} \
	Boxer::~Boxer() {}
//-----------------------------------------------------------------

#define FEBIRD_BOXING_OBJECT_DERIVE(Boxer, Boxed) \
	class Boxer : public obj, public Boxed { public: \
		Boxer(); \
		explicit Boxer(const Boxed& y); \
		~Boxer(); \
		const Boxer& get_boxed() const { return *this; } \
		      Boxer& get_boxed()       { return *this; } \
	}; \
	template<> struct obj_val<Boxed> { typedef Boxer type; }; \
	template<> struct obj_val<Boxer> { typedef Boxer type; };
//-----------------------------------------------------------------
#define FEBIRD_BOXING_OBJECT_DERIVE_IMPL(Boxer, Boxed) \
	Boxer::Boxer() {} \
	Boxer::Boxer(const Boxed& y) : Boxed(y) {} \
	Boxer::~Boxer() {}
//-----------------------------------------------------------------

	FEBIRD_BOXING_OBJECT(obj_bool   ,   bool)
	FEBIRD_BOXING_OBJECT(obj_short  ,   signed short)
	FEBIRD_BOXING_OBJECT(obj_ushort , unsigned short)
	FEBIRD_BOXING_OBJECT(obj_int    ,   signed int)
	FEBIRD_BOXING_OBJECT(obj_uint   , unsigned int)
	FEBIRD_BOXING_OBJECT(obj_long   ,   signed long)
	FEBIRD_BOXING_OBJECT(obj_ulong  , unsigned long)
	FEBIRD_BOXING_OBJECT(obj_llong  ,   signed long long)
	FEBIRD_BOXING_OBJECT(obj_ullong , unsigned long long)
	FEBIRD_BOXING_OBJECT(obj_float  , float)
	FEBIRD_BOXING_OBJECT(obj_double , double)
	FEBIRD_BOXING_OBJECT(obj_ldouble, long double)
	FEBIRD_BOXING_OBJECT_DERIVE(obj_string, std::string)
	FEBIRD_BOXING_OBJECT_DERIVE(obj_array , valvec<obj_ptr>)

} } // namespace febird::objbox

#endif // __febird_objbox_hpp__

