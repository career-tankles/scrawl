#ifndef __penglei_valvec_hpp__
#define __penglei_valvec_hpp__

#include <assert.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include <iterator>
#include <memory>
#include <stdexcept>
#include <functional>
#include <utility>

#include <boost/type_traits.hpp>
#include <boost/static_assert.hpp>

//
#if defined(__GNUC__) && __GNUC__ >= 4
	#include <ext/memory> // for uninitialized_copy_n
	#define STDEXT_uninitialized_copy_n __gnu_cxx::uninitialized_copy_n
#else
	#define STDEXT_uninitialized_copy_n std::uninitialized_copy_n
#endif

#if defined(__GXX_EXPERIMENTAL_CXX0X__) || __cplusplus >= 201103L
	#include <initializer_list>
	#ifndef HSM_HAS_MOVE
		#define HSM_HAS_MOVE
	#endif
#endif

template<class T>
inline bool is_object_overlap(const T* x, const T* y) {
	assert(NULL != x);
	assert(NULL != y);
	if (x+1 <= y || y+1 <= x)
		return false;
	else
		return true;
}

/// similary with std::vector, but:
///   1. use realloc to enlarge/shrink memory, this has avoid memcpy when
///      realloc is implemented by mremap for large chunk of memory;
///      mainstream realloc are implemented in this way
///   2. valvec also avoid calling copy-cons when enlarge the valvec
///@Note
///  1. T must be memmove-able, std::list,map,set... are not memmove-able
///  2. std::vector, string, ... are memmove-able, they could be the T
template<class T>
class valvec {
protected:
    T*     p;
    size_t n;
    size_t c; // capacity

	template<class InputIter>
	void construct(InputIter first, ptrdiff_t count) {
		assert(count >= 0);
		if (count) {
			p = (T*)malloc(sizeof(T) * count);
			if (NULL == p) throw std::bad_alloc();
			STDEXT_uninitialized_copy_n(first, count, p);
		} else {
            p = NULL;
		}
		n = c = count;
	}

	template<class> struct void_ { typedef void type; };

	template<class InputIter>
	void construct(InputIter first, InputIter last, std::input_iterator_tag) {
		p = NULL;
		n = c = 0;
		for (; first != last; ++first)
			this->push_back(*first);
	}
	template<class ForwardIter>
	void construct(ForwardIter first, ForwardIter last, std::forward_iterator_tag) {
		ptrdiff_t count = std::distance(first, last);
		assert(count >= 0);
		construct(first, count);
	}

	template<class InputIter>
	void assign_aux(InputIter first, InputIter last, std::input_iterator_tag) {
		resize(0);
		for (; first != last; ++first)
			this->push_back(*first);
	}
	template<class ForwardIter>
	void assign_aux(ForwardIter first, ForwardIter last, std::forward_iterator_tag) {
		ptrdiff_t count = std::distance(first, last);
		assign(first, count);
	}

public:
    typedef T  value_type;
    typedef T* iterator;
    typedef T& reference;
    typedef const T* const_iterator;
    typedef const T& const_reference;

    typedef std::reverse_iterator<T*> reverse_iterator;
    typedef std::reverse_iterator<const T*> const_reverse_iterator;

    typedef size_t size_type;
    typedef ptrdiff_t difference_type;

    T* begin() { return p; }
    T* end()   { return p + n; }

    const T* begin() const { return p; }
    const T* end()   const { return p + n; }

    T* rbegin() { return p + n; }
    T* rend()   { return p; }

    const T* rbegin() const { return p + n; }
    const T* rend()   const { return p; }

    const T* cbegin() const { return p; }
    const T* cend()   const { return p + n; }

    const T* crbegin() const { return p + n; }
    const T* crend()   const { return p; }

    valvec() {
        p = NULL;
        n = c = 0;
    }

    explicit valvec(size_t sz, const T& val = T()) {
        if (sz) {
            p = (T*)malloc(sizeof(T) * sz);
            if (NULL == p) throw std::bad_alloc();
            std::uninitialized_fill_n(p, sz, val);
            n = c = sz;
        } else {
            p = NULL;
            n = c = 0;
        }
    }

	template<class AnyIter>
	valvec(const std::pair<AnyIter, AnyIter>& rng
		, typename std::iterator_traits<AnyIter>::iterator_category tag
		= typename std::iterator_traits<AnyIter>::iterator_category()) {
		construct(rng.first, rng.second(), tag);
	}
	template<class AnyIter>
	valvec(AnyIter first, AnyIter last
		, typename std::iterator_traits<AnyIter>::iterator_category tag
		= typename std::iterator_traits<AnyIter>::iterator_category()) 	{
		construct(first, last, tag);
	}
	template<class InputIter>
	valvec(InputIter first, ptrdiff_t count
		, typename std::iterator_traits<InputIter>::iterator_category
		= typename std::iterator_traits<InputIter>::iterator_category()) 	{
		assert(count >= 0);
		construct(first, count);
	}

	valvec(const valvec& y) {
        assert(this != &y);
		assert(!is_object_overlap(this, &y));
		construct(y.p, y.n);
    }

    valvec& operator=(const valvec& y) {
        if (&y == this)
            return *this;
		assert(!is_object_overlap(this, &y));
        clear();
		construct(y.p, y.n);
        return *this;
    }

#ifdef HSM_HAS_MOVE
    valvec(valvec&& y) {
        assert(this != &y);
		assert(!is_object_overlap(this, &y));
		p = y.p;
		n = y.n;
		c = y.c;
		y.p = NULL;
		y.n = 0;
		y.c = 0;
	}

	valvec& operator=(valvec&& y) {
        assert(this != &y);
        if (&y == this)
            return *this;
		assert(!is_object_overlap(this, &y));
		this->clear();
		new(this)valvec(y);
		return *this;
	}
#endif // HSM_HAS_MOVE

    ~valvec() { clear(); }

    void fill(const T& x) {
        std::fill_n(p, n, x);
    }

    template<class AnyIter>
	typename void_<typename std::iterator_traits<AnyIter>::iterator_category>::type
    assign(const std::pair<AnyIter, AnyIter>& rng) {
		assign_aux(rng.first, rng.second, typename std::iterator_traits<AnyIter>::iterator_category());
    }
    template<class AnyIter>
	typename void_<typename std::iterator_traits<AnyIter>::iterator_category>::type
    assign(AnyIter first, AnyIter last) {
		assign_aux(first, last, typename std::iterator_traits<AnyIter>::iterator_category());
    }
    template<class InputIter>
	typename void_<typename std::iterator_traits<InputIter>::iterator_category>::type
    assign(InputIter first, ptrdiff_t len) {
		assert(len >= 0);
		erase_all();
        ensure_capacity(len);
		STDEXT_uninitialized_copy_n(first, len, p);
		n = len;
    }

    void clear() {
        if (p) {
            std::_Destroy(p, p+n);
            free(p);
        }
        p = NULL;
        n = c = 0;
    }

	const T* data() const { return p; }
	      T* data()       { return p; }

    bool  empty() const { return 0 == n; }
    size_t size() const { return n; }
    size_t capacity() const { return c; }

    void reserve(size_t newcap) {
        if (newcap <= c)
            return; // nothing to do
        T* q = (T*)realloc(p, sizeof(T) * newcap);
        if (NULL == q) throw std::bad_alloc();
        p = q;
        c = newcap;
    }

    void ensure_capacity(size_t min_cap) {
        size_t hard_min_cap = 1;
        if (min_cap < hard_min_cap)
            min_cap = hard_min_cap;
        if (min_cap <= c) {
            // nothing to do
            return;
        }
        if (min_cap < 2 * c)
            min_cap = 2 * c;
        T* q = (T*)realloc(p, sizeof(T) * min_cap);
        if (NULL == q) throw std::bad_alloc();
        p = q;
        c = min_cap;
    }

    void shrink_to_fit() {
        assert(n <= c);
        if (n == c)
            return;
        if (n) {
            T* q = (T*)realloc(p, sizeof(T) * n);
            if (NULL == q) throw std::bad_alloc();
            p = q;
            c = n;
        } else {
            if (p)
                free(p);
            p = NULL;
            c = n = 0;
        }
    }

    void resize(size_t newsize, const T& val = T()) {
        if (newsize == n)
            return; // nothing to do
        if (newsize < n)
            std::_Destroy(p+newsize, p+n);
        else {
            ensure_capacity(newsize);
            std::uninitialized_fill_n(p+n, newsize-n, val);
        }
        n = newsize;
    }

	void erase_all() {
		std::_Destroy(p, p+n);
		n = 0;
	}

    // client code should pay the risk for performance gain
    void resize_no_init(size_t newsize) {
    //  assert(boost::has_trivial_constructor<T>::value);
        ensure_capacity(newsize);
        n = newsize;
    }
    // client code should pay the risk for performance gain
	void resize0() { n = 0; }

	///  trim [from, end)
	void trim(T* from) {
		assert(from >= p);
		assert(from <= p + n);
        std::_Destroy(from, p+n);
		n = from - p;
	}

	void insert(const T* pos, const T& x) {
		assert(pos <= p + n);
		assert(pos >= p);
		insert(pos-p, x);
	}

	void insert(size_t pos, const T& x) {
		assert(pos <= n);
		if (pos > n) {
			throw std::out_of_range("valvec::insert");
		}
        if (n == c)
            ensure_capacity(n+1);
	//	for (ptrdiff_t i = n; i > pos; --i) memcpy(p+i, p+i-1, sizeof T);
		memmove(p+pos+1, p+pos, sizeof(T)*(n-pos));
		new(p+pos)T(x);
		++n;
	}

    void push_back(const T& x = T()) {
        if (n == c)
            ensure_capacity(n+1);
        new(p+n)T(x); // copy cons
        ++n;
    }
	template<class Iterator>
	void append(Iterator first, ptrdiff_t len) {
		assert(len >= 0);
        size_t newsize = n + len;
        ensure_capacity(newsize);
        STDEXT_uninitialized_copy_n(first, len, p+n);
        n = newsize;
	}
	template<class Iterator>
	void append(Iterator first, Iterator last) {
		ptrdiff_t len = std::distance(first, last);
        size_t newsize = n + len;
        ensure_capacity(newsize);
        std::uninitialized_copy(first, last, p+n);
        n = newsize;
	}
	template<class Container>
	void append(const Container& cont, typename Container::value_type* = NULL, typename Container::const_iterator* =NULL) {
		append(cont.begin(), cont.end());
	}
    void unchecked_push_back(const T& x = T()) {
		assert(n < c);
        new(p+n)T(x); // copy cons
        ++n;
    }
    void pop_back() {
        if (0 == n)
            throw std::logic_error("valvec::pop_back(), already empty");
        p[n-1].~T();
        --n;
    }
    void unchecked_pop_back() {
        assert(n > 0);
        p[n-1].~T();
        --n;
    }

    const T& operator[](size_t i) const {
        assert(i < n);
        return p[i];
    }

    T& operator[](size_t i) {
        assert(i < n);
        return p[i];
    }

    const T& at(size_t i) const {
        if (i >= n) throw std::out_of_range("valvec::at");
        return p[i];
    }

    T& at(size_t i) {
        if (i >= n) throw std::out_of_range("valvec::at");
        return p[i];
    }

    const T& front() const {
        assert(n);
        assert(p);
        return p[0];
    }
    T& front() {
        assert(n);
        assert(p);
        return p[0];
    }

    const T& back() const {
        assert(n);
        assert(p);
        return p[n-1];
    }
    T& back() {
        assert(n);
        assert(p);
        return p[n-1];
    }

    T& ende(size_t d) {
        assert(d <= n);
        return p[n-d];
    }
    const T& ende(size_t d) const {
        assert(d <= n);
        return p[n-d];
    }

    void operator+=(const T& x) {
        push_back(x);
    }

    void operator+=(const valvec& y) {
        size_t newsize = n + y.size();
        ensure_capacity(newsize);
        std::uninitialized_copy(y.p, y.p + y.n, p+n);
        n = newsize;
    }

    void swap(valvec& y) {
        std::swap(p, y.p);
        std::swap(n, y.n);
        std::swap(c, y.c);
    }

#if defined(__GXX_EXPERIMENTAL_CXX0X__) || __cplusplus >= 201103
	template<class... Args>
	void emplace_back(Args&&... args) {
        if (n == c)
            ensure_capacity(n+1);
        new(p+n)T(std::forward<Args>(args)...);
        ++n;
	}
	explicit valvec(std::initializer_list<T> list) {
		construct(list.begin(), list.size());
	}
#else
	template<class A1>
	void emplace_back(const A1& a1) {
        if (n == c)
            ensure_capacity(n+1);
        new(p+n)T(a1);
        ++n;
	}
	template<class A1, class A2>
	void emplace_back(const A1& a1, const A2& a2) {
        if (n == c)
            ensure_capacity(n+1);
        new(p+n)T(a1, a2);
        ++n;
	}
	template<class A1, class A2, class A3>
	void emplace_back(const A1& a1, const A2& a2, const A3& a3) {
        if (n == c)
            ensure_capacity(n+1);
        new(p+n)T(a1, a2, a3);
        ++n;
	}
	template<class A1, class A2, class A3, class A4>
	void emplace_back(const A1& a1, const A2& a2, const A3& a3, const A4& a4) {
        if (n == c)
            ensure_capacity(n+1);
        new(p+n)T(a1, a2, a3, a4);
        ++n;
	}
#endif

	std::pair<T*, T*> range() { return std::make_pair(p, p+n); }
	std::pair<const T*, const T*> range() const { return std::make_pair(p, p+n); }

	const T& get_2d(size_t colsize, size_t row, size_t col) const {
		size_t idx = row * colsize + col;
		assert(idx < n);
		return p[idx];
	}
	T& get_2d(size_t col_size, size_t row, size_t col) {
		size_t idx = row * col_size + col;
		assert(idx < n);
		return p[idx];
	}

	void risk_set_data(T* data) { p = data; }
	void risk_set_size(size_t size) { this->n = size; }
	void risk_set_capacity(size_t capa) { this->c = capa; }
/*
 * Now serialization valvec<T> has been builtin DataIO
 *
	template<class DataIO>
	friend void DataIO_saveObject(DataIO& dio, const valvec& x) {
		typename DataIO::my_var_uint64_t size(x.n);
		dio << size;
		// complex object has not implemented yet!
		BOOST_STATIC_ASSERT(boost::has_trivial_destructor<T>::value);
		dio.ensureWrite(x.p, sizeof(T)*x.n);
	}

   	template<class DataIO>
	friend void DataIO_loadObject(DataIO& dio, valvec& x) {
		typename DataIO::my_var_uint64_t size;
		dio >> size;
		x.resize_no_init(size.t);
		// complex object has not implemented yet!
		BOOST_STATIC_ASSERT(boost::has_trivial_destructor<T>::value);
		dio.ensureRead(x.p, sizeof(T)*x.n);
	}
*/

	static bool lessThan(const valvec& x, const valvec& y) {
		return lessThan(x, y, std::less<T>(), std::equal_to<T>());
    }
    static bool equalTo(const valvec& x, const valvec& y) {
		return equalTo(x, y, std::equal_to<T>());
    }
    template<class TLess, class TEqual>
    static bool lessThan(const valvec& x, const valvec& y, TLess le, TEqual eq) {
        size_t n = std::min(x.n, y.n);
        for (size_t i = 0; i < n; ++i) {
            if (!eq(x.p[i], y.p[i]))
                return le(x.p[i], y.p[i]);
        }
        return x.n < y.n;
    }
    template<class TEqual>
    static bool equalTo(const valvec& x, const valvec& y, TEqual eq) {
        if (x.n != y.n)
            return false;
        for (size_t i = 0, n = x.n; i < n; ++i) {
            if (!eq(x.p[i], y.p[i]))
                return false;
        }
        return true;
    }
    template<class TLess = std::less<T>, class TEqual = std::equal_to<T> >
    struct less : TLess, TEqual {
        bool operator()(const valvec& x, const valvec& y) const {
            return lessThan<TLess, TEqual>(x, y, *this, *this);
        }
        less() {}
        less(TLess le, TEqual eq) : TLess(le), TEqual(eq) {}
    };
    template<class TEqual = std::equal_to<T> >
    struct equal : TEqual {
        bool operator()(const valvec& x, const valvec& y) const {
            return equalTo<TEqual>(x, y, *this);
        }
        equal() {}
        equal(TEqual eq) : TEqual(eq) {}
    };
};

namespace std {
	template<class T>
	void swap(valvec<T>& x, valvec<T>& y) { x.swap(y); }
}

#endif

