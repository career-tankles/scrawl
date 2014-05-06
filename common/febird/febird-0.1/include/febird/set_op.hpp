/* vim: set tabstop=4 : */
/********************************************************************
	@file set_op.hpp
	@brief set operations

	@date	2006-10-23 14:02
	@author	leipeng
	@{
*********************************************************************/
#ifndef __febird_set_op_h__
#define __febird_set_op_h__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
# pragma once
#endif

// #include <boost/preprocessor/iteration/local.hpp>
// #include <boost/preprocessor/enum.hpp>
// #include <boost/preprocessor/enum_params.hpp>
// #include <boost/mpl/at.hpp>
// #include <boost/mpl/map.hpp>
//

#include <assert.h>
#include <vector>
#include <algorithm>
#include <functional>
#include <stdexcept>
#include <boost/tuple/tuple.hpp>
#include <boost/type_traits.hpp>
#include <boost/mpl/if.hpp>
#include <boost/multi_index/identity.hpp>
#include "stdtypes.hpp"
#include "util/compare.hpp"

namespace febird {
//@{
/**
 @brief multi set intersection -- result is copy from [_First1, _Last1)

 [dest, result) is [_First1, _Last1) AND [_First2, _Last2)

 predictions:
   for any [x1, x2) in [_First1, _Last1), _Pred(x2, x1) = false,
   that is say: x1 <= x2
   allow same key in sequence
 @note
  - result is copy from [_First1, _Last1)
  - values in sequence can be equal, when values are equal, multi values will be copied
  - _InIt1::value_type, _InIt2::value_type, _OutIt::value_type need not be same
  - _InIt1::value_type and _InIt2::value_type must comparable by _Pr
  - _InIt1::value_type must assignable to _OutIt::value_type
  - _InIt2::value_type need not assignable to _OutIt::value_type
 */
template<class _InIt1, class _InIt2, class _OutIt, class _Pr>
inline
_OutIt multiset_intersection(_InIt1 _First1, _InIt1 _Last1,
							 _InIt2 _First2, _InIt2 _Last2, _OutIt dest, _Pr _Pred)
{
	for (; _First1 != _Last1 && _First2 != _Last2; )
	{
		if (_Pred(*_First1, *_First2))
			++_First1;
		else if (_Pred(*_First2, *_First1))
			++_First2;
		else
			*dest++ = *_First1++; // do not increment _First2
	}
	return (dest);
}

template<class _InIt1, class _RandInIt2, class _OutIt, class _Pr>
inline
_OutIt multiset_1small_intersection(_InIt1 _First1, _InIt1 _Last1,
									_RandInIt2 _First2, _RandInIt2 _Last2, _OutIt dest, _Pr _Pred)
{
	for (; _First1 != _Last1 && _First2 != _Last2; )
	{
		std::pair<_RandInIt2, _RandInIt2> range = std::equal_range(_First2, _Last2, *_First1, _Pred);
		if (range.first == range.second)
			++_First1;
		else {
			// !(*range.first < *_First1) --> *_First1 <= *range.first
			while (_First1 != _Last1 && !_Pred(*range.first, *_First1))
				*dest++ = *_First1++;
		}
		_First2 = range.second;
	}
	return (dest);
}

template<class _InIt1, class _RandInIt2, class _OutIt, class _Pr>
inline
_OutIt multiset_fast_intersection(_InIt1 _First1, _InIt1 _Last1,
								  _RandInIt2 _First2, _RandInIt2 _Last2, _OutIt dest, _Pr _Pred, int threshold = 32)
{
	if (std::distance(_First1, _Last1) * threshold < std::distance(_First2, _Last2))
		return multiset_1small_intersection(_First1, _Last1, _First2, _Last2, dest, _Pred);
	else
		return multiset_intersection(_First1, _Last1, _First2, _Last2, dest, _Pred);
}
//@}

//////////////////////////////////////////////////////////////////////////

//@{
/**
 @brief multi set intersection2 -- result is copy from [_First2, _Last2)

 [dest, result) is [_First1, _Last1) AND [_First2, _Last2)
 for any [x1, x2) in [_First1, _Last1), _Pred(x2, x1) = false
 allow same key in sequence

 @note
  - result is copy from [_First2, _Last2)
  - values in sequence can be equal, when values are equal, multi values will be copied
  - _InIt1::value_type, _InIt2::value_type, _OutIt::value_type need not be same
  - _InIt1::value_type and _InIt2::value_type must comparable by _Pr
  - _InIt2::value_type must assignable to _OutIt::value_type
  - _InIt1::value_type need not assignable to _OutIt::value_type
 */
template<class _InIt1, class _InIt2, class _OutIt, class _Pr>
inline
_OutIt multiset_intersection2(_InIt1 _First1, _InIt1 _Last1,
							  _InIt2 _First2, _InIt2 _Last2, _OutIt dest, _Pr _Pred)
{
	for (; _First1 != _Last1 && _First2 != _Last2; )
	{
		if (_Pred(*_First1, *_First2))
			++_First1;
		else if (_Pred(*_First2, *_First1))
			++_First2;
		else
			*dest++ = *_First2++; // do not increment _First1
	}
	return (dest);
}

template<class _InIt1, class _RandInIt2, class _OutIt, class _Pr>
inline
_OutIt multiset_1small_intersection2(_InIt1 _First1, _InIt1 _Last1,
									 _RandInIt2 _First2, _RandInIt2 _Last2, _OutIt dest, _Pr _Pred)
{
	for (; _First1 != _Last1 && _First2 != _Last2; )
	{
		std::pair<_RandInIt2, _RandInIt2> range = std::equal_range(_First2, _Last2, *_First1, _Pred);
		if (range.first != range.second)
		{
			dest = std::copy(range.first, range.second, dest);
		}
		_First2 = range.second;
		++_First1;
	}
	return (dest);
}

template<class _RandInIt1, class _RandInIt2, class _OutIt, class _Pr>
inline
_OutIt multiset_fast_intersection2(_RandInIt1 _First1, _RandInIt1 _Last1,
								   _RandInIt2 _First2, _RandInIt2 _Last2, _OutIt dest, _Pr _Pred, int threshold = 32)
{
	if (std::distance(_First1, _Last1) * threshold < std::distance(_First2, _Last2))
		return multiset_1small_intersection2(_First1, _Last1, _First2, _Last2, dest, _Pred);
	else
		return multiset_intersection2(_First1, _Last1, _First2, _Last2, dest, _Pred);
}
//@}

//! 一定是调用 pred(prev, curr), 而不是 pred(curr, prev)
//!
//! 可以保证对有序序列 pred(prev, curr) 等效于 !equal_to(prev, curr)
//! 从而可以对有序序列使用 set_unique(first, last, febird::not2(less_than_comp))
//! 来删除重复元素
//!
template<class _FwdIt, class _Pr> inline
_FwdIt set_unique(_FwdIt _First, _FwdIt _Last, _Pr _Pred)
{	// remove each matching previous
	for (_FwdIt _Firstb; (_Firstb = _First) != _Last && ++_First != _Last; )
		if (_Pred(*_Firstb, *_First))
			{	// copy down
			for (; ++_First != _Last; )
				if (!_Pred(*_Firstb, *_First))
					*++_Firstb = *_First;
			return (++_Firstb);
			}
	return (_Last);
}

template<class _FwdIt> inline
	_FwdIt set_unique(_FwdIt _First, _FwdIt _Last)
{
	return set_unique(_First, _Last,
		std::equal_to<typename std::iterator_traits<_FwdIt>::value_type>()
		);
}

template<class _Pr>
class not1_functor
{
	_Pr m_pr;

public:
	typedef bool result_type;

	explicit not1_functor(_Pr _Pred) : m_pr(_Pred) {}
	not1_functor() {}

	template<class T1>
	bool operator()(const T1& x) const
	{
		return !m_pr(x);
	}
};

template<class _Pr>
class not2_functor
{
	_Pr m_pr;

public:
	typedef bool result_type;

	explicit not2_functor(_Pr _Pred) : m_pr(_Pred) {}
	not2_functor() {}

	template<class T1, class T2>
	bool operator()(const T1& x, const T2& y) const
	{
		return !m_pr(x, y);
	}
};

template<class _Pr>
not1_functor<_Pr> not1(const _Pr& _Pred)
{
	return not1_functor<_Pr>(_Pred);
}
template<class _Pr>
not2_functor<_Pr> not2(const _Pr& _Pred)
{
	return not2_functor<_Pr>(_Pred);
}

//////////////////////////////////////////////////////////////////////////
//! find in predicted near range, if not found, find it in the whole range
//!
template<class IterT, class CompareT>
IterT find_next_larger(IterT first, IterT prediction, IterT last, CompareT comp)
{
	IterT iter = std::upper_bound(first, prediction, *first, comp);
	if (iter == prediction && !comp(*first, *prediction)) // *first == *prediction
		return std::upper_bound(prediction, last, *first, comp);
	else
		return iter;
}

//////////////////////////////////////////////////////////////////////////

//! Call Convert(*iter) to convert the iterator
template<class InputIter, class ConvertedType, class Convertor>
class convertor_iterator_adaptor :
	public std::iterator< typename std::iterator_traits<InputIter>::iterator_category
						, ConvertedType>
{
	typedef convertor_iterator_adaptor<InputIter, ConvertedType, Convertor> self_t;

	Convertor m_convert;

public:
	InputIter m_iter;

	typedef typename std::iterator_traits<InputIter>::difference_type difference_type;

	explicit convertor_iterator_adaptor(InputIter iter, const Convertor& convert = Convertor())
		: m_iter(iter), m_convert(convert) {}

	ConvertedType operator*() const { return m_convert(*m_iter); }

	self_t& operator++() { ++m_iter; return *this; }
	self_t& operator--() { --m_iter; return *this; }

	self_t operator++(int) { self_t temp(*this); ++m_iter; return temp; }
	self_t operator--(int) { self_t temp(*this); --m_iter; return temp; }

	bool operator==(const self_t& y) const { return m_iter == y.m_iter; }
	bool operator!=(const self_t& y) const { return m_iter != y.m_iter; }

	bool operator<=(const self_t& y) const { return m_iter <= y.m_iter; }
	bool operator>=(const self_t& y) const { return m_iter >= y.m_iter; }

	bool operator<(const self_t& y) const { return m_iter < y.m_iter; }
	bool operator>(const self_t& y) const { return m_iter > y.m_iter; }

	difference_type operator-(const self_t& y) const { return m_iter - y.m_iter; }

	self_t  operator+(difference_type y) const { return self_t(m_iter + y, m_convert); }

	self_t& operator+=(difference_type i) { m_iter += i; return *this; }
	self_t& operator-=(difference_type i) { m_iter -= i; return *this; }
};
template<class ConvertedType, class InputIter, class Convertor>
convertor_iterator_adaptor<InputIter, ConvertedType, Convertor>
convert_iterator(InputIter iter, const Convertor& convertor)
{
	return convertor_iterator_adaptor<InputIter, ConvertedType, Convertor>(iter, convertor);
}

//! Call extractor(iter) to convert source iter to target iter
//!
//! if *iter is something computed from iter, the adapter can avoid to calling *iter
//! @note
//!  -# Extractor is applied on iter, not '*iter'
//!  -# Extractor should be a functor
template<class InputIter, class MemberType, class Extractor>
class member_iterator_adaptor
  : public std::iterator< typename std::iterator_traits<InputIter>::iterator_category
						, MemberType>
{
	typedef member_iterator_adaptor<InputIter, MemberType, Extractor> self_t;

private:
	Extractor m_extr;

public:
	InputIter m_iter;

	typedef typename std::iterator_traits<InputIter>::difference_type difference_type;

	explicit member_iterator_adaptor(InputIter iter, const Extractor& convert = Extractor())
		: m_iter(iter), m_extr(convert) {}

	MemberType operator*() const { return m_extr(m_iter); }

	self_t& operator++() { ++m_iter; return *this; }
	self_t& operator--() { --m_iter; return *this; }

	self_t operator++(int) { self_t temp(*this); ++m_iter; return temp; }
	self_t operator--(int) { self_t temp(*this); --m_iter; return temp; }

	bool operator==(const self_t& y) const { return m_iter == y.m_iter; }
	bool operator!=(const self_t& y) const { return m_iter != y.m_iter; }

	bool operator<=(const self_t& y) const { return m_iter <= y.m_iter; }
	bool operator>=(const self_t& y) const { return m_iter >= y.m_iter; }

	bool operator<(const self_t& y) const { return m_iter < y.m_iter; }
	bool operator>(const self_t& y) const { return m_iter > y.m_iter; }

	difference_type operator-(const self_t& y) const { return m_iter - y.m_iter; }

	self_t  operator+(difference_type y) const { return self_t(m_iter + y, m_extr); }

	self_t& operator+=(difference_type i) { m_iter += i; return *this; }
	self_t& operator-=(difference_type i) { m_iter -= i; return *this; }
};
template<class MemberType, class InputIter, class Extractor>
member_iterator_adaptor<InputIter, MemberType, Extractor>
member_iterator(InputIter iter, const Extractor& convertor)
{
	return member_iterator_adaptor<InputIter, MemberType, Extractor>(iter, convertor);
}
//////////////////////////////////////////////////////////////////////////

template<class Iter>
typename std::iterator_traits<Iter>::value_type
sum(Iter first, Iter last)
{
	typename std::iterator_traits<Iter>::value_type s =
	typename std::iterator_traits<Iter>::value_type();
	for (; first != last; ++first)
		s += *first;
	return s;
}
template<class C>
typename C::value_type
sum(const C& c)
{
	return sum(c.begin(), c.end());
}
template<class Iter, class Extractor>
typename Extractor::key_type
sum(Iter first, Iter last, Extractor ex)
{
	typename Extractor::key_type s = typename Extractor::key_type();
	for (; first != last; ++first)
		s += ex(*first);
	return s;
}
template<class C, class Extractor>
typename Extractor::key_type
sum(const C& c, Extractor ex)
{
	return sum(c.begin(), c.end(), ex);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////

template <class RandomAccessIterator, class Distance, class T, class Compare>
void febird_push_heap(RandomAccessIterator first, Distance holeIndex,
					  Distance topIndex, T value, Compare comp)
{
	Distance parent = (holeIndex - 1) / 2;
	while (holeIndex > topIndex && comp(*(first + parent), value)) {
		*(first + holeIndex) = *(first + parent);
		holeIndex = parent;
		parent = (holeIndex - 1) / 2;
	}
	*(first + holeIndex) = value;
}

template <class RandomAccessIterator, class Distance, class T, class Compare>
void febird_adjust_heap(RandomAccessIterator first, Distance holeIndex,
						Distance len, T value, Compare comp)
{
	Distance topIndex = holeIndex;
	Distance secondChild = 2 * holeIndex + 2;
	while (secondChild < len) {
		if (comp(*(first + secondChild), *(first + (secondChild - 1))))
			secondChild--;
		*(first + holeIndex) = *(first + secondChild);
		holeIndex = secondChild;
		secondChild = 2 * (secondChild + 1);
	}
	if (secondChild == len) {
		*(first + holeIndex) = *(first + (secondChild - 1));
		holeIndex = secondChild - 1;
	}
	febird_push_heap(first, holeIndex, topIndex, value, comp);
}

template <class RandomAccessIterator, class Compare>
inline void pop_heap_ignore_top(RandomAccessIterator first, RandomAccessIterator last, Compare comp)
{
	if (febird_likely(last - first > 1))
	{
		typedef typename std::iterator_traits<RandomAccessIterator>::difference_type diff_t;
		febird_adjust_heap(first, diff_t(0), diff_t(last - first - 1), *(last-1), comp);
	}
}

template <class RandomAccessIterator, class Compare>
void adjust_heap_top(RandomAccessIterator first, RandomAccessIterator last, Compare comp)
{
	if (febird_likely(last - first > 1))
	{
		typedef typename std::iterator_traits<RandomAccessIterator>::difference_type diff_t;
		febird_adjust_heap(first, diff_t(0), diff_t(last - first), *first, comp);
	}
}
//---------------------------------------------------------------------------
//
// adjust heap from bottom/larger index to top/small index
template <class RandomAccessIterator, class Distance, class T, class Compare, class UpdateIndex>
void febird_push_heap(RandomAccessIterator first, Distance holeIndex,
					  Distance topIndex, T value, Compare comp, UpdateIndex update)
{
	Distance parent = (holeIndex - 1) / 2;
	while (holeIndex > topIndex && comp(*(first + parent), value)) {
		*(first + holeIndex) = *(first + parent);
		update(*(first + holeIndex), holeIndex);
		holeIndex = parent;
		parent = (holeIndex - 1) / 2;
	}
	*(first + holeIndex) = value;
	update(*(first + holeIndex), holeIndex);
}

// update first[holeIndex] in heap `[first, first + len)` with new value `value`, and ajust the heap
template <class RandomAccessIterator, class Distance, class T, class Compare, class UpdateIndex>
void febird_adjust_heap(RandomAccessIterator first, Distance holeIndex,
						Distance len, T value, Compare comp, UpdateIndex update)
{
	Distance topIndex = holeIndex;
	Distance secondChild = 2 * holeIndex + 2;
	while (secondChild < len) {
		if (comp(*(first + secondChild), *(first + (secondChild - 1))))
			secondChild--;
		*(first + holeIndex) = *(first + secondChild);
		update(*(first + holeIndex), holeIndex);
		holeIndex = secondChild;
		secondChild = 2 * (secondChild + 1);
	}
	if (secondChild == len) {
		*(first + holeIndex) = *(first + (secondChild - 1));
		update(*(first + holeIndex), holeIndex);
		holeIndex = secondChild - 1;
	}
	febird_push_heap(first, holeIndex, topIndex, value, comp, update);
}

// first[holeIndex] has updated, adjust the heap
template <class RandomAccessIterator, class Distance, class Compare, class UpdateIndex>
inline void
adjust_heap_hole(RandomAccessIterator first, RandomAccessIterator last, Distance holeIndex,
				 Compare comp, UpdateIndex update)
{
	febird_adjust_heap(first, holeIndex, Distance(last - first), *(first + holeIndex), comp, update);
}

// first[0] is hole, and will be lost, not same with std::pop_heap
// std::pop_heap will move first[0] to last[-1], pop_heap_ignore_top will not
// after this function call, last[-1] will become gabage, you should remove it
// @code
//		vector<T*> vec;
//		...
//		pop_heap_ignore_top(vec.begin(), vec.end(), comp, UpdateIndex());
//		vec.pop_back(); // remove last[-1]
// @endcode
template <class RandomAccessIterator, class Compare, class UpdateIndex>
inline void 
pop_heap_ignore_top(RandomAccessIterator first, RandomAccessIterator last, Compare comp, UpdateIndex update)
{
	if (last - first > 1)
	{
		typedef typename std::iterator_traits<RandomAccessIterator>::difference_type diff_t;
		febird_adjust_heap(first, diff_t(0), diff_t(last - first - 1), *(last-1), comp, update);
	}
}

// first[0] has updated, adjust the heap to cordinate new value of first[0]
template <class RandomAccessIterator, class Compare, class UpdateIndex>
inline void 
adjust_heap_top(RandomAccessIterator first, RandomAccessIterator last, Compare comp, UpdateIndex update)
{
	if (last - first > 1)
	{
		typedef typename std::iterator_traits<RandomAccessIterator>::difference_type diff_t;
		febird_adjust_heap(first, diff_t(0), diff_t(last - first), *first, comp, update);
	}
}

//////////////////////////////////////////////////////////////////////////
/**
 @brief 
 - like std::remove_if, but use std::swap
 */
template<class ForwIter, class Pred>
ForwIter remove_swap_if(ForwIter first, ForwIter last, Pred pred)
{
	for (; first != last; ++first)
		if (pred(*first))
			goto DoErase;
	return last;
DoErase: {
	ForwIter dest = first;
	for (++first; first != last; ++first)
	{
		using std::swap;
		if (!pred(*first))
			swap(*dest, *first), ++dest;
	}
	return dest;
  }
}

namespace multi_way
{

/**
 @brief can be better inline when pass by param
 */
class FirstEqualSecond
{
public:
	template<class Iter>
	bool operator()(const std::pair<Iter,Iter>& ii) const
	{
		return ii.first == ii.second;
	}
};

class MustProvideKeyExtractor;

enum CacheLevel {
	cache_default, //!< default select
	cache_none,
	cache_key,
	cache_value
};
typedef boost::integral_constant<CacheLevel, cache_none > tag_cache_none;
typedef boost::integral_constant<CacheLevel, cache_key  > tag_cache_key;
typedef boost::integral_constant<CacheLevel, cache_value> tag_cache_value;

template<class InputIter, class KeyType, class KeyExtractor, CacheLevel HowCache>
class CacheStrategy; // template proto-type definition

template<class InputIter, class KeyType, class KeyExtractor>
class CacheStrategy<InputIter, KeyType, KeyExtractor, cache_none>
{
public:
	typedef typename std::iterator_traits<InputIter>::value_type value_type;
	typedef tag_cache_none           cache_category;
	typedef boost::tuples::null_type cache_item_type;

protected:
	//! 这个函数必须要带 cache_category 参数
	//! 对 cache_none, 这个函数是在 derived class 中实现的
	//! 但是，因为 derived class 中使用了 using super::get_cache_key, 所以这里必须有 get_cache_key 成员
	inline void get_cache_key(int nth/*, tag_cache_none*/) const
	{
		// do nothing...
		// should implement in derived class
		// BOOST_STATIC_ASSERT(0);
	}

	inline KeyType key_from_cache_item(const KeyType& key) const
	{
		return key;
	}

	inline void input_cache_item(int i, const InputIter&)
	{
		// do nothing...
	}

	inline void resize_cache(int size)
	{
		// do nothing...
	}

//  has no this function
// 	void set_cache_item(int i, const cache_item_type&)
// 	{
// 		// do nothing...
// 	}

protected:
//	cache_container_type m_cache;
	KeyExtractor         m_key_extractor;
};

template<class InputIter, class KeyType, class KeyExtractor>
class CacheStrategy<InputIter, KeyType, KeyExtractor, cache_key>
{
public:
	typedef typename std::iterator_traits<InputIter>::value_type value_type;
	typedef tag_cache_key           cache_category;
	typedef KeyType                 cache_item_type;

protected:
	//! 这个函数必须要带 cache_category 参数
	inline const KeyType& get_cache_key(int nth, tag_cache_key) const
	{
		return m_cache[nth];
	}

	inline const KeyType& key_from_cache_item(const KeyType& key) const
	{
		return key;
	}

	inline void input_cache_item(int i, InputIter iter)
	{
		m_cache[i] = m_key_extractor(*iter);
	}

	inline void resize_cache(int size)
	{
		m_cache.resize(size);
	}

	void set_cache_item(int i, const KeyType& item)
	{
		m_cache[i] = item;
	}

protected:
	std::vector<KeyType> m_cache;
	KeyExtractor         m_key_extractor;
};

template<class InputIter, class KeyType, class KeyExtractor>
class CacheStrategy<InputIter, KeyType, KeyExtractor, cache_value>
{
public:
	typedef typename std::iterator_traits<InputIter>::value_type value_type;
	typedef tag_cache_value           cache_category;
	typedef value_type                cache_item_type;

protected:
	//! 这个函数必须要带 cache_category 参数
	inline const KeyType& get_cache_key(int nth, tag_cache_value) const
	{
		return m_key_extractor(m_cache[nth]);
	}

	inline const KeyType& key_from_cache_item(const value_type& value) const
	{
		return m_key_extractor(value);
	}

	inline void input_cache_item(int i, InputIter iter)
	{
		m_cache[i] = *iter;
	}

	inline void resize_cache(int size)
	{
		m_cache.resize(size);
	}

	void set_cache_item(int i, const value_type& item)
	{
		m_cache[i] = item;
	}

protected:
	std::vector<value_type> m_cache;
	KeyExtractor            m_key_extractor;
};

template<class InputIter, class KeyType, class KeyExtractor>
class CacheStrategy<InputIter, KeyType, KeyExtractor, cache_default>
	/**
	 @brief cache_item_type is KeyType or value_type

	 为了 cache 的效率，仅缓存必要的数据(CPU cache, or disk cache)
	 - input_iterator_tag 表示同一个数据只从 iterator 读一次，因此缓存整个 value
	 - forward_iterator_tag 及更高级，可以允许从 iterator 读多次，因此只缓存 key
	 */
	: public boost::mpl::if_c<
		boost::is_same<std::input_iterator_tag,
					   typename std::iterator_traits<InputIter>::iterator_category
					  >::value,
		CacheStrategy<InputIter, KeyType, KeyExtractor, cache_value>,
		CacheStrategy<InputIter, KeyType, KeyExtractor, cache_key>
	>::type
{
};

template<class Value, class Key, class MultiWay>
class MultiWay_SetOP : public std::iterator<std::input_iterator_tag, Value, const Value*, const Value&>
{
public:
	typedef Key     key_type;
	typedef Value value_type;

	const value_type& operator*() const
	{
		return static_cast<const MultiWay*>(this)->current_value();
	}
	const value_type* operator->() const
	{
		return &static_cast<const MultiWay*>(this)->current_value();
	}
	MultiWay& operator++()
	{
		static_cast<MultiWay*>(this)->increment();
		return *static_cast<MultiWay*>(this);
	}
private:
	void operator++(int); //!< disabled

public:
	bool comp_value(const value_type& x, const value_type& y) const
	{
		return static_cast<const MultiWay*>(this)->m_comp(
			   static_cast<const MultiWay*>(this)->m_key_extractor(x),
			   static_cast<const MultiWay*>(this)->m_key_extractor(y));
	}

	bool comp_key(const key_type& x, const key_type& y) const
	{
		return static_cast<const MultiWay*>(this)->m_comp(x, y);
	}

	/**
	 @brief 求多个集合的和集，相当于合并多个有序序列

	 这个操作对序列的要求有所放宽，允许每个序列中的元素可以重复
	 @see intersection
	 */
	template<class _OutIt> _OutIt merge(_OutIt dest)
	{
		for (; !static_cast<MultiWay*>(this)->is_end(); ++*this, ++dest)
		{
			*dest = **this;
		}
		return dest;
	}

	template<class _OutIt> _OutIt copy(_OutIt dest) { return merge(dest); }

	template<class _OutIt, class _Cond>
	_OutIt copy_if(_OutIt dest, _Cond cond)
	{
		while (!static_cast<MultiWay*>(this)->is_end())
		{
			value_type x = **this;
			if (cond(x))
				*dest = x, ++dest;
		}
		return dest;
	}

	/**
	 @brief 按条件拷贝个有序集合的元素

	 @param cond   过滤条件，调用形式: cond(n, x, y), n 个与 x 相等的 value

	 @note
		-# 如果 MultiWay 不是 LoserTree
	      -# 多个输入中必须【有且只有一个】输入包含【可合法访问】的无穷大元素
		-# 多个序列的总长度至少为 2(包含无穷大元素)

	 @see intersection
	 */
	template<class _OutIt, class _Cond>
	_OutIt copy_if2(_OutIt dest, _Cond cond)
	{
		assert(!static_cast<MultiWay*>(this)->is_end());

		value_type x = **this;
		for (;;)
		{
			size_t n = 0;
			bool bEqual;
			do {
				++n; ++*this;
				if (febird_unlikely(static_cast<MultiWay*>(this)->is_end()))
					return dest;
				value_type y = **this;
				if (cond(n, x, y))
					*dest++ = x;
				bEqual = !comp_value(x, y);
				x = y;
			} while (bEqual);
		}
		return dest;
	}

	template<class _OutIt, class _Cond>
	_OutIt copy_if2_eofm(_OutIt dest, _Cond cond, const key_type& eofm)
	{
		assert(!static_cast<MultiWay*>(this)->is_end());

		value_type x = **this;
		assert(comp_key(static_cast<MultiWay*>(this)->m_extractor(x), eofm));
		do {
			size_t n = 0;
			bool bEqual;
			do {
				++n; ++*this;
				value_type y = **this;
				if (cond(n, x, y))
					*dest++ = x;
				bEqual = !comp_value(x, y);
				x = y;
			} while (bEqual);

		} while (comp_key(static_cast<MultiWay*>(this)->m_extractor(x), eofm));

		return dest;
	}

	template<class _OutIt> _OutIt copy_equal(_OutIt dest)
	{
		return copy_equal(dest, boost::multi_index::identity<value_type>());
	}

	//! @return equal count with param[in] x
	//!
	//! @note after calling this function, *this is point to next larger value
	//!
	template<class _OutIt, class DataExtractor>
	_OutIt copy_equal(_OutIt dest, DataExtractor dataEx)
	{
		assert(!static_cast<MultiWay*>(this)->is_end());

		const value_type x = **this;
		const key_type k = static_cast<MultiWay*>(this)->m_key_extractor(x);
		value_type y = x;
		do {
			*dest = dataEx(y); ++dest;
			++*this;
			if (febird_unlikely(static_cast<MultiWay*>(this)->is_end()))
				break;
			y = **this;
		} while (!comp_key(k, static_cast<MultiWay*>(this)->m_key_extractor(y)));

		return dest;
	}

	//! not need param eofm
	//! this function can not assert
	template<class _OutIt>
	_OutIt copy_equal_eofm(_OutIt dest)
	{
		return copy_equal_eofm(dest, boost::multi_index::identity<value_type>());
	//	copy_equal_eofm(dest, XIdentity());
	}

	//! @return next pos of dest
	//!
	//! @note after calling this function, *this is point to next larger value
	//!
	template<class _OutIt, class DataExtractor>
	_OutIt copy_equal_eofm(_OutIt dest, DataExtractor dataEx)
	{
		assert(!static_cast<MultiWay*>(this)->is_end());

		const value_type x = **this;
		const key_type k = static_cast<MultiWay*>(this)->m_key_extractor(x);
		value_type y = x;
		do {
			*dest = dataEx(y); ++dest;
			++*this;
			y = **this;
		} while (!comp_key(k, static_cast<MultiWay*>(this)->m_key_extractor(y)));

		return dest;
	}

	size_t skip_equal()
	{
		assert(!static_cast<MultiWay*>(this)->is_end());

		key_type k = static_cast<MultiWay*>(this)->m_key_extractor(**this);
		size_t n = 0;
		do {
			++*this; ++n;
		} while (!static_cast<MultiWay*>(this)->is_end() &&
				 !comp_key(k, static_cast<MultiWay*>(this)->m_key_extractor(**this)));
		return n;
	}

	size_t skip_equal_eofm()
	{
		assert(!static_cast<MultiWay*>(this)->is_end());
		
		key_type k = static_cast<MultiWay*>(this)->m_key_extractor(**this);
		size_t n = 0;
		do {
			++*this; ++n;
		} while (!comp_key(k, static_cast<MultiWay*>(this)->m_key_extractor(**this)));

		return n;
	}

	/**
	 @brief 求多个集合的交集

	 @param dest  将结果拷贝到这里

	 @note
		-# 如果 MultiWay 是 LoserTree
	      -# 每个输入的末尾必须是无穷大元素
	 */
	template<class _OutIt> _OutIt intersection(_OutIt dest)
	{
		assert(!static_cast<MultiWay*>(this)->is_end());
		if (static_cast<MultiWay*>(this)->total_ways() <= 32)
			return intersection_32(dest);
		else
			return intersection_n(dest);
	}

	template<class _OutIt> _OutIt intersection_eofm(_OutIt dest, const key_type& eofm)
	{
		assert(!static_cast<MultiWay*>(this)->is_end());
		if (static_cast<MultiWay*>(this)->total_ways() <= 32)
			return intersection_eofm_32(dest, eofm);
		else
			return intersection_eofm_n(dest, eofm);
	}
private:
	/**
	 @note
		-# 如果 MultiWay 是 LoserTree
	      -# 每个输入的末尾必须是无穷大元素
		-# 每个序列不能有重复元素
	 */
	template<class _OutIt> _OutIt intersection_n(_OutIt dest)
	{
		if (static_cast<MultiWay*>(this)->is_end())
			return dest;
		size_t ways = static_cast<MultiWay*>(this)->total_ways();
		key_type  x = static_cast<MultiWay*>(this)->m_key_extractor(**this);
		for (;;)
		{
			key_type y;
			size_t i = 0;
			do {
				++*this; ++i;
				if (febird_unlikely(static_cast<MultiWay*>(this)->is_end()))
				{
					if (febird_unlikely(i == ways))
						*dest = x, ++dest;
					return dest;
				}
				y = static_cast<MultiWay*>(this)->m_key_extractor(**this);

			} while (!comp_key(x, y));

			if (febird_unlikely(i == ways))
				*dest = x, ++dest;
			x = y;
		}
	}

	/**
	 @note
		-# 每个序列可以有重复元素
	 */
	template<class _OutIt> _OutIt intersection_32(_OutIt dest)
	{
		if (static_cast<MultiWay*>(this)->is_end())
			return dest;
		const uint32_t mask = 0xFFFFFFFF >> (32-static_cast<MultiWay*>(this)->total_ways());
		key_type x = static_cast<MultiWay*>(this)->m_key_extractor(**this);
		for (;;)
		{
			uint32_t cur_mask = 0;
			key_type y;
			do {
				cur_mask |= 1 << static_cast<MultiWay*>(this)->current_way();
				++*this;
				if (static_cast<MultiWay*>(this)->is_end())
				{
					if (mask == cur_mask)
						*dest = x, ++dest;
					return dest;
				}
				y = static_cast<MultiWay*>(this)->m_key_extractor(**this);

			} while (!comp_key(x, y));

			if (mask == cur_mask)
				*dest = x, ++dest;
			x = y;
		}
	}

	/**
	 @note
	    -# 每个输入的末尾必须是无穷大元素 eofm
		-# 每个序列不能有重复元素
	 */
	template<class _OutIt> _OutIt intersection_eofm_n(_OutIt dest, const key_type& eofm)
	{
		if (static_cast<MultiWay*>(this)->is_end())
			return dest;
		size_t ways = static_cast<MultiWay*>(this)->total_ways();
		key_type  x = static_cast<MultiWay*>(this)->m_key_extractor(**this);
		assert(comp_key(x, eofm));
		do {
			key_type y;
			size_t i = 0;
			do {
				++*this; ++i;
				y = static_cast<MultiWay*>(this)->m_key_extractor(**this);

			} while (!comp_key(x, y));

			if (febird_unlikely(i == ways))
				*dest = x, ++dest;
			x = y;
		} while (comp_key(x, eofm));

		return dest;
	}

	/**
	 @note
		-# 如果 MultiWay 是 LoserTree
	      -# 每个输入的末尾必须是无穷大元素
		-# 每个序列可以有重复元素
	 */
	template<class _OutIt> _OutIt intersection_eofm_32(_OutIt dest, const key_type& eofm)
	{
		if (static_cast<MultiWay*>(this)->is_end())
			return dest;
		const uint32_t mask = 0xFFFFFFFF >> (32-static_cast<MultiWay*>(this)->total_ways());
		key_type x = static_cast<MultiWay*>(this)->m_key_extractor(**this);
		assert(comp_key(x, eofm));
		do {
			uint32_t cur_mask = 0;
			key_type y;
			do {
				cur_mask |= 1 << static_cast<MultiWay*>(this)->current_way();
				++*this;
				y = static_cast<MultiWay*>(this)->m_key_extractor(**this);

			} while (!comp_key(x, y));

			if (mask == cur_mask)
				*dest = x, ++dest;
			x = y;
		} while (comp_key(x, eofm));

		return dest;
	}

public:
	/**
	 @brief 求多个集合的交集的尺寸

	 @note
		-# 如果 MultiWay 是 LoserTree
	      -# 每个输入的末尾必须是无穷大元素
		-# 每个序列不能有重复元素
	 */
	size_t intersection_size()
	{
		if (static_cast<MultiWay*>(this)->is_end())
			return 0;
		key_type x = static_cast<MultiWay*>(this)->m_key_extractor(**this);
		size_t n = 0;
		size_t ways = static_cast<MultiWay*>(this)->total_ways();
		for (;;)
		{
			key_type y = x;
			size_t i = 0;
			do {
				++*this; ++i;
				if (static_cast<MultiWay*>(this)->is_end())
				{
					if (static_cast<MultiWay*>(this)->total_ways() == i)
						++n;
					return n;
				}
				y = static_cast<MultiWay*>(this)->m_key_extractor(**this);

			} while (!comp_key(x, y));

			if (ways == i)
				++n;
			x = y;
		}
	}
	size_t intersection_size_eofm(const key_type& eofm)
	{
		if (static_cast<MultiWay*>(this)->is_end())
			return 0;
		key_type x = static_cast<MultiWay*>(this)->m_key_extractor(**this);
		size_t n = 0;
		size_t ways = static_cast<MultiWay*>(this)->total_ways();
		assert(comp_key(x, eofm));
		do {
			key_type y = x;
			size_t i = 0;
			do {
				++*this; ++i;
				y = static_cast<MultiWay*>(this)->m_key_extractor(**this);

			} while (!comp_key(x, y));

			if (ways == i)
				++n;
			x = y;
		} while (!static_cast<MultiWay*>(this)->is_end());
	}

	template<class _OutIt> _OutIt unique(_OutIt dest)	{ return union_set(dest); }
	template<class _OutIt> _OutIt unique_eofm(_OutIt dest, const key_type& eofm) { return union_set_eofm(dest, eofm); }

	size_t unique_size() { return union_size(); }
	size_t unique_size_eofm(const key_type& eofm) { return union_size_eofm(eofm); }

	/**
	 @brief 求多个集合的并集

	 @note
		-# 如果 MultiWay 不是 LoserTree
	      -# 多个输入中必须【有且只有一个】输入包含【可合法访问】的无穷大元素
		-# 多个序列的总长度至少为 2(包含无穷大元素)

	 @see intersection
	 */
	template<class _OutIt> _OutIt union_set(_OutIt dest)
	{
		if (static_cast<MultiWay*>(this)->is_end())
			return dest;
		key_type x = static_cast<MultiWay*>(this)->m_key_extractor(**this);
		for (;;)
		{
			key_type y = x;
			do {
				++*this;
				if (febird_unlikely(static_cast<MultiWay*>(this)->is_end()))
				{
					*dest++ = x;
					return dest;
				}
				y = static_cast<MultiWay*>(this)->m_key_extractor(**this);

			} while (!comp_key(x, y));

			*dest++ = x;
			x = y;
		}
	}

	template<class _OutIt> _OutIt union_set_eofm(_OutIt dest, const key_type& eofm)
	{
		if (static_cast<MultiWay*>(this)->is_end())
			return dest;
		key_type x = static_cast<MultiWay*>(this)->m_key_extractor(**this);
		assert(comp_key(x, eofm));
		do {
			key_type y;
			do {
				++*this;
				y = static_cast<MultiWay*>(this)->m_key_extractor(**this);

			} while (!comp_key(x, y));

			*dest++ = x;
			x = y;
		} while (comp_key(x, eofm));

		return dest;
	}

	/**
	 @brief 求多个集合的并集的尺寸

	 @note
		-# 如果 MultiWay 不是 LoserTree
	      -# 多个输入中必须【有且只有一个】输入包含【可合法访问】的无穷大元素
		-# 多个序列的总长度至少为 2(包含无穷大元素)

	 @see union
	 */
	size_t union_size()
	{
		if (static_cast<MultiWay*>(this)->is_end())
			return 0;
		key_type x = static_cast<MultiWay*>(this)->m_key_extractor(**this);
		size_t n = 0;
		for (;;)
		{
			key_type y;
			do {
				++*this;
				if (febird_unlikely(static_cast<MultiWay*>(this)->is_end()))
					return ++n;
				y = static_cast<MultiWay*>(this)->m_key_extractor(**this);

			} while (!comp_key(x, y));

			++n;
			x = y;
		}
	}

	size_t union_size_eofm(const key_type& eofm)
	{
		if (static_cast<MultiWay*>(this)->is_end())
			return 0;
		key_type x = static_cast<MultiWay*>(this)->m_key_extractor(**this);
		size_t n = 0;
		assert(comp_key(x, eofm));
		do {
			key_type y;
			do {
				++*this;
				y = static_cast<MultiWay*>(this)->m_key_extractor(**this);

			} while (!comp_key(x, y));
			++n;
			x = y;
		} while (comp_key(x, eofm));

		return n;
	}

	//! KeyToEqualCountIter::value_type 必须可以从 std::pair<key_type, size_t> 隐式转化
	//!
	template<class KeyToEqualCountIter>
	KeyToEqualCountIter key_to_equal_count(KeyToEqualCountIter iter)
	{
		if (static_cast<MultiWay*>(this)->is_end())
			return iter;
		key_type x = static_cast<MultiWay*>(this)->m_key_extractor(**this);
		for (;;)
		{
			key_type y;
			size_t n = 0;
			do {
				++*this; ++n;
				if (febird_unlikely(static_cast<MultiWay*>(this)->is_end()))
				{
					*iter = std::pair<key_type, size_t>(x, n); ++iter;
					return iter;
				}
				y = static_cast<MultiWay*>(this)->m_key_extractor(**this);
			} while (!comp_key(x, y));

			*iter = std::pair<key_type, size_t>(x, n); ++iter;
			x = y;
		}
	}

	//! KeyToEqualCountIter::value_type 必须可以从 std::pair<key_type, size_t> 隐式转化
	//! @param minEqualCount only output keys which happen minEqualCount times
	template<class KeyToEqualCountIter>
	KeyToEqualCountIter key_to_equal_count(KeyToEqualCountIter iter, size_t minEqualCount)
	{
		if (febird_unlikely(static_cast<MultiWay*>(this)->is_end()))
			return iter;
		key_type x = static_cast<MultiWay*>(this)->m_key_extractor(**this);
		for (;;)
		{
			key_type y;
			size_t n = 0;
			do {
				++*this; ++n;
				if (febird_unlikely(static_cast<MultiWay*>(this)->is_end()))
				{
					if (n >= minEqualCount)
						*iter = std::pair<key_type, size_t>(x, n), ++iter;
					return iter;
				}
				y = static_cast<MultiWay*>(this)->m_key_extractor(**this);

			} while (!comp_key(x, y));

			if (n >= minEqualCount)
				*iter = std::pair<key_type, size_t>(x, n), ++iter;
			x = y;
		}
	}

	//! KeyToEqualCountIter::value_type 必须可以从 std::pair<key_type, size_t> 隐式转化
	//!
	template<class KeyToEqualCountIter>
	KeyToEqualCountIter key_to_equal_count_eofm(KeyToEqualCountIter iter, const key_type& eofm)
	{
		if (febird_unlikely(static_cast<MultiWay*>(this)->is_end()))
			return iter;
		key_type x = static_cast<MultiWay*>(this)->m_key_extractor(**this);
		assert(comp_key(x, eofm));
		do {
			key_type y;
			size_t n = 0;
			do {
				++*this; ++n;
				y = static_cast<MultiWay*>(this)->m_key_extractor(**this);
			} while (!comp_key(x, y));

			*iter = std::pair<key_type, size_t>(x, n); ++iter;
			x = y;
		} while (comp_key(x, eofm));

		return iter;
	}

	//! KeyToEqualCountIter::value_type 必须可以从 std::pair<key_type, size_t> 隐式转化
	//! @param minEqualCount only output keys which happen minEqualCount times
	template<class KeyToEqualCountIter>
	KeyToEqualCountIter key_to_equal_count_eofm(KeyToEqualCountIter iter, size_t minEqualCount, const key_type& eofm)
	{
		if (febird_unlikely(static_cast<MultiWay*>(this)->is_end()))
			return iter;
		key_type x = static_cast<MultiWay*>(this)->m_key_extractor(**this);
		assert(comp_key(x, eofm));
		do {
			key_type y;
			size_t n = 0;
			do {
				++*this; ++n;
				y = static_cast<MultiWay*>(this)->m_key_extractor(**this);
			} while (!comp_key(x, y));

			if (n >= minEqualCount)
				*iter = std::pair<key_type, size_t>(x, n), ++iter;
			x = y;
		} while (comp_key(x, eofm));

		return iter;
	}
};

//#define LT_WAYS(Container) std::vector<Container::const_iterator>

#define LT_iiter_traits typename std::iterator_traits<typename std::iterator_traits<RandIterOfInput>::value_type>

template< class RandIterOfInput

		, class KeyType = LT_iiter_traits::value_type

		, bool StableSort = false //!< same Key in various way will output by way order

		, class Compare = std::less<KeyType>

		, class KeyExtractor = typename boost::mpl::if_c<
				boost::is_same<KeyType,
							   LT_iiter_traits::value_type
							  >::value,
				boost::multi_index::identity<KeyType>,
				MustProvideKeyExtractor
			>::type

	    , CacheLevel HowCache = cache_default
		>
class LoserTree :
	public CacheStrategy< typename std::iterator_traits<RandIterOfInput>::value_type
						, KeyType
						, KeyExtractor
						, HowCache
						>,
	public MultiWay_SetOP< LT_iiter_traits::value_type
						 , KeyType
						 , LoserTree<RandIterOfInput, KeyType, StableSort, Compare, KeyExtractor, HowCache>
						 >
{
	DECLARE_NONE_COPYABLE_CLASS(LoserTree)

	typedef CacheStrategy< typename std::iterator_traits<RandIterOfInput>::value_type
						, KeyType
						, KeyExtractor
						, HowCache
						>
	super;

	friend class MultiWay_SetOP< LT_iiter_traits::value_type
						 , KeyType
						 , LoserTree<RandIterOfInput, KeyType, StableSort, Compare, KeyExtractor, HowCache>
						 >;
public:
	typedef typename std::iterator_traits<RandIterOfInput>::value_type way_iter_t;
	typedef typename std::iterator_traits<way_iter_t     >::value_type value_type;
	typedef KeyType  key_type;
	typedef KeyExtractor key_extractor;
	typedef boost::integral_constant<bool, StableSort> is_stable_sort;

	typedef typename super::cache_category  cache_category;
	typedef typename super::cache_item_type cache_item_type;

public:
	/**
	 @brief construct

	 @par 图示如下：
	 @code

RandIterOfInput                                          this is guard value
      ||                                                          ||
	  ||                                                          ||
	  \/                                                          \/
	 first--> 0 way_iter_t [min_value.........................max_value]
	   /      1 way_iter_t [min_value.........................max_value] <--- 每个序列均已
	   |      2 way_iter_t [min_value.........................max_value]      按 comp 排序
	   |      3 way_iter_t [min_value.........................max_value]
	  <       4 way_iter_t [min_value.........................max_value]
	   |      5 way_iter_t [min_value.........................max_value]
	   |      7 way_iter_t [min_value.........................max_value]
	   \      8 way_iter_t [min_value.........................max_value]
	 last---> end

	 @endcode

	 @param comp    value 的比较器

	 @note 每个序列最后必须要有一个 max_value 作为序列结束标志，否则会导致未定义行为
	 */
	LoserTree(RandIterOfInput first, RandIterOfInput last,
			  const KeyType& max_key,
			  const Compare& comp = Compare(),
			  const KeyExtractor& keyExtractor = KeyExtractor())
	{
		init(first, last, max_key, comp, keyExtractor);
	}
	LoserTree(RandIterOfInput first, int length,
			  const KeyType& max_key,
			  const Compare& comp = Compare(),
			  const KeyExtractor& keyExtractor = KeyExtractor())
	{
		init(first, first + length, max_key, comp, keyExtractor);
	}

// 	LoserTree(RandIterOfInput first, RandIterOfInput last,
// 			  const cache_item_type& min_item,
// 			  const cache_item_type& max_item,
// 			  const KeyType& max_key,
// 			  const Compare& comp = Compare(),
// 			  const KeyExtractor& keyExtractor = KeyExtractor())
// 	{
// 		init_yan_wu(first, last, min_item, max_item, comp, keyExtractor);
// 	}
// 	LoserTree(RandIterOfInput first, int length,
// 			  const cache_item_type& min_item, // yan_wu init need
// 			  const cache_item_type& max_item,
// 			  const KeyType& max_key,
// 			  const Compare& comp = Compare(),
// 			  const KeyExtractor& keyExtractor = KeyExtractor())
// 	{
// 		init_yan_wu(first, first + length, min_item, max_item, comp, keyExtractor);
// 	}

	LoserTree()
	{
	}

	/**
	 @brief 初始化

	- 共有 n 个内部结点，n 个外部结点
	- winner 只用于初始化时计算败者树，算完后即丢弃
	- winner/loser 的第 0 个单元都不是内部结点，不属于树中的一员
	- winner 的第 0 个单元未用
	- m_tree 的第 0 个单元用于保存最终的赢者, 其它单元保存败者

	- 该初始化需要的 n-1 次比较，总的时间复杂度是 O(n)

	- 严蔚敏&吴伟民 的 LoserTree 初始化复杂度是 O(n*log(n))，并且还需要一个 min_key,
	  但是他们的初始化不需要额外的 winner 数组

    - 并且，这个实现比 严蔚敏&吴伟民 的 LoserTree 初始化更强壮
	 */
	void init(RandIterOfInput first, RandIterOfInput last,
			  const KeyType& max_key,
			  const Compare& comp = Compare(),
			  const KeyExtractor& keyExtractor = KeyExtractor())
	{
		this->m_comp = comp;
		this->m_key_extractor = keyExtractor;

		m_beg = first;
		m_end = last;

		m_max_key = max_key;

		int len = int(last - first);
		if (febird_unlikely(0 == len))
		{
			throw std::invalid_argument("LoserTree: way sequence must not be empty");
		}

		m_tree.resize(len);

		this->resize_cache(len);

		int i;
		for (i = 0; i != len; ++i)
		{
			// read first value from every sequence
			this->input_cache_item(i, *(first+i));
		}
		if (febird_unlikely(1 == len))
		{
			m_tree[0] = 0;
			return;
		}

		int minInnerToEx = len / 2;

		std::vector<int> winner(len);

		for (i = len - 1; i > minInnerToEx; --i)
		{
			exter_loser_winner(m_tree[i], winner[i], i, len);
		}
		int left, right;
		if (len & 1) // odd
		{ // left child is last inner node, right child is first external node
			left = winner[len-1];
			right = 0;
		}
		else
		{
			left = 0;
			right = 1;
		}
		get_loser_winner(m_tree[minInnerToEx], winner[minInnerToEx], left, right);

		for (i = minInnerToEx; i > 0; i /= 2)
		{
			for (int j = i-1; j >= i/2; --j)
			{
				inner_loser_winner(m_tree[j], winner[j], j, winner);
			}
		}
		m_tree[0] = winner[1];
	}

	//! 严蔚敏&吴伟民 的 LoserTree 初始化
	//! 复杂度是 O(n*log(n))，并且还需要一个 min_key
	void init_yan_wu(RandIterOfInput first, RandIterOfInput last,
					 const cache_item_type& min_item,
					 const cache_item_type& max_item,
					 const Compare& comp = Compare(),
					 const KeyExtractor& keyExtractor = KeyExtractor())
	{
		//! this function do not support cache_none
		BOOST_STATIC_ASSERT(HowCache != cache_none);

		assert(first < last); // ensure that will not construct empty loser tree

		this->m_comp = comp;
		this->m_key_extractor = keyExtractor;

		m_beg = first;
		m_end = last;

		m_max_key = this->key_from_cache_item(max_item);

		int len = int(last - first);
		m_tree.resize(len);

		this->resize_cache(len+1);
		this->set_cache_item(len, min_item);

		int i;
		for (i = 0; i != len; ++i)
		{
			m_tree[i] = len;

			// read first value from every sequence
			this->input_cache_item(i, *(first+i));
		}
		for (i = len-1; i >= 0; --i)
			ajust(i);

		// 防止 cache 的最后一个成员上升到 top ??.....
		//
		this->set_cache_item(len, max_item);

	//	assert(!m_tree.empty());
	//	if (m_tree[0] == len)
	//		ajust(len); // 会导致在 ajust 中 m_tree[parent] 越界
	}

	const value_type& current_value() const
	{
		assert(!m_tree.empty());
	//	assert(!is_end()); // 允许访问末尾的 guardValue, 便于简化 app
		return current_value_aux(cache_category());
	}

	/**
	 @brief return current way NO.
	 */
	int current_way() const
	{
		assert(!m_tree.empty());
	//	assert(!is_end()); // allow this use
		return m_tree[0];
	}

	size_t total_ways() const
	{
		return m_tree.size();
	}

	bool is_any_way_end() const
	{
		return is_end();
	}

	bool is_end() const
	{
		assert(!m_tree.empty());
		const KeyType& cur_key = get_cache_key(m_tree[0], cache_category());
		return !m_comp(cur_key, m_max_key); // cur_key >= max_value
	}

	void increment()
	{
		assert(!m_tree.empty());
		assert(!is_end());
		int top = m_tree[0];
		input_cache_item(top, ++*(m_beg + top));
		ajust(top);
	}

	void ajust_for_update_top()
	{
		assert(!m_tree.empty());
		int top = m_tree[0];
		input_cache_item(top, *(m_beg + top));
		ajust(top);
	}

	way_iter_t& top()
	{
		assert(!m_tree.empty());
		return *(m_beg + m_tree[0]);
	}

	void reserve(int maxTreeSize)
	{
		m_tree.reserve(maxTreeSize);
		this->resize_cache(maxTreeSize);
	}

	template<class _OutIt, class _Cond>
	_OutIt copy_if2(_OutIt dest, _Cond cond, const key_type& eofm) { this->copy_if2_eofm(dest, cond, m_max_key); }

	template<class _OutIt>
	_OutIt copy_equal(_OutIt dest) { return this->copy_equal_eofm(dest, boost::multi_index::identity<value_type>()); }
	template<class _OutIt, class DataExtractor>
	_OutIt copy_equal(_OutIt dest, DataExtractor dataEx) { return this->copy_equal_eofm(dest, dataEx); }

	size_t skip_equal()	{ return this->skip_equal_eofm(); }

	template<class _OutIt>
	_OutIt intersection(_OutIt dest) { return this->intersection_eofm(dest, m_max_key); }
	size_t intersection_size() { return this->intersection_size_eofm(m_max_key); }

	template<class _OutIt>
	_OutIt union_set(_OutIt dest) { return this->union_set_eofm(dest, m_max_key); }
	size_t union_size() { return this->union_size_eofm(m_max_key); }

	template<class _OutIt>
	_OutIt unique(_OutIt dest) { return this->union_set_eofm(dest, m_max_key); }
	size_t unique_size() { return this->union_size_eofm(m_max_key); }

	template<class KeyToEqualCountIter>
	KeyToEqualCountIter key_to_equal_count(KeyToEqualCountIter iter) { return this->key_to_equal_count_eofm(iter, m_max_key); }
	template<class KeyToEqualCountIter>
	KeyToEqualCountIter key_to_equal_count(KeyToEqualCountIter iter, size_t minEqualCount)
	{ return this->key_to_equal_count_eofm(iter, minEqualCount, m_max_key); }

protected:
	void ajust(int s)
	{
		int parent = int(s + m_tree.size()) >> 1;
		while (parent > 0)
		{
			if (comp_cache_item(m_tree[parent], s, cache_category(), is_stable_sort()))
			{
				std::swap(s, m_tree[parent]);
			}
			parent >>= 1;
		}
		m_tree[0] = s;
	}

	void exter_loser_winner(int& loser, int& winner, int parent, int len) const
	{
		int left  = 2 * parent - len;
		int right = left + 1;
		get_loser_winner(loser, winner, left, right);
	}
	void inner_loser_winner(int& loser, int& winner, int parent, const std::vector<int>& winner_vec) const
	{
		int left  = 2 * parent;
		int right = 2 * parent + 1;
		left = winner_vec[left];
		right = winner_vec[right];
		get_loser_winner(loser, winner, left, right);
	}
	void get_loser_winner(int& loser, int& winner, int left, int right) const
	{
		if (comp_cache_item(left, right, cache_category(), is_stable_sort()))
		{
			loser  = right;
			winner = left;
		}
		else
		{
			loser = left;
			winner = right;
		}
	}

	const value_type& current_value_aux(tag_cache_none) const
	{
		assert(m_tree[0] < int(m_tree.size()));
		return **(m_beg + m_tree[0]);
	}
	const value_type& current_value_aux(tag_cache_key) const
	{
		assert(m_tree[0] < int(m_tree.size()));
		return **(m_beg + m_tree[0]);
	}
	const value_type& current_value_aux(tag_cache_value) const
	{
		assert(m_tree[0] < int(m_tree.size()));
		return this->m_cache[m_tree[0]];
	}

	using super::get_cache_key;
	inline const KeyType get_cache_key(int nth, tag_cache_none) const
	{
		return this->m_key_extractor(**(m_beg + nth));
	}

	template<class CacheCategory>
	inline bool comp_cache_item(int x, int y,
								CacheCategory cache_tag,
								boost::mpl::true_ isStableSort) const
	{
		return comp_key_stable(x, y,
				get_cache_key(x, cache_tag),
				get_cache_key(y, cache_tag),
				typename HasTriCompare<Compare>::type());
	}

	bool comp_key_stable(int x, int y, const KeyType& kx, const KeyType& ky,
						 boost::mpl::true_ hasTriCompare) const
	{
		int ret = m_comp.compare(kx, ky);
		if (ret < 0)
			return true;
		if (ret > 0)
			return false;
		ret = m_comp.compare(kx, m_max_key);
		assert(ret <= 0);
		if (0 == ret)
			return false;
		else
			return x < y;
	}
	bool comp_key_stable(int x, int y, const KeyType& kx, const KeyType& ky,
						 boost::mpl::false_ hasTriCompare) const
	{
		if (m_comp(kx, ky))
			return true;
		if (m_comp(ky, kx))
			return false;
		if (!m_comp(kx, m_max_key)) // kx >= max_key --> kx == max_key
		{ // max_key is the max, so must assert this:
			assert(!m_comp(m_max_key, kx));
			return false;
		}
		else return x < y;
	}

	template<class CacheCategory>
	inline bool comp_cache_item(int x, int y,
								CacheCategory cache_tag,
								boost::mpl::false_ isStableSort) const
	{
		return m_comp(get_cache_key(x, cache_tag), get_cache_key(y, cache_tag));
	}

protected:
	KeyType  m_max_key;
	std::vector<int> m_tree;
	RandIterOfInput  m_beg;
	RandIterOfInput  m_end;
	Compare          m_comp;
};

template<class _InIt>
struct Heap_WayIterEx : public std::pair<_InIt, _InIt>
{
	int index;

	template<class _InIt2>
	Heap_WayIterEx(const Heap_WayIterEx<_InIt2>& other)
		: std::pair<_InIt, _InIt>(other.first, other.second), index(other.index)
	{
	}

	Heap_WayIterEx(int index, const _InIt& i1, const _InIt& i2)
		: std::pair<_InIt, _InIt>(i1, i2), index(index)
	{
	}
};

template<class _InIt>
Heap_WayIterEx<_InIt> make_way_iter_ex(int index, const _InIt& i1, const _InIt& i2)
{
	return Heap_WayIterEx<_InIt>(index, i1, i2);
}

template< class _InIt

		, class KeyType = typename std::iterator_traits<_InIt>::value_type

		, bool StableSort = false //!< same Key in various way will output by way order

		, class Compare = std::less<KeyType>

		, class KeyExtractor = typename boost::mpl::if_c<
				boost::is_same<KeyType,
							   typename std::iterator_traits<_InIt>::value_type
							  >::value,
				boost::multi_index::identity<KeyType>,
				MustProvideKeyExtractor
			>::type

		, class WayIter = typename boost::mpl::if_c<StableSort, Heap_WayIterEx<_InIt>, std::pair<_InIt, _InIt> >::type
		>
class HeapMultiWay_TrivalWayIter :
	public MultiWay_SetOP< typename std::iterator_traits<_InIt>::value_type
						 , KeyType
						 , HeapMultiWay_TrivalWayIter<_InIt, KeyType, StableSort, Compare, KeyExtractor, WayIter>
						 >
{
	DECLARE_NONE_COPYABLE_CLASS(HeapMultiWay_TrivalWayIter)

	friend class MultiWay_SetOP< typename std::iterator_traits<_InIt>::value_type
							   , KeyType
							   , HeapMultiWay_TrivalWayIter<_InIt, KeyType, StableSort, Compare, KeyExtractor, WayIter>
							   >;
public:
	typedef typename std::iterator_traits<_InIt>::value_type value_type;
	typedef KeyExtractor key_extractor;
	typedef KeyType  key_type;
	typedef Compare  compare_t;
	typedef WayIter  way_iter_t;
	typedef std::vector<way_iter_t> way_vec_t;
	typedef boost::integral_constant<bool, StableSort> is_stable_sort;

	class HeapCompare
	{
		const HeapMultiWay_TrivalWayIter& heap;
	public:
		explicit HeapCompare(const HeapMultiWay_TrivalWayIter& heap) : heap(heap) {}

		//! 反转输入参数，用做堆操作的比较
		inline bool operator()(const way_iter_t& left, const way_iter_t& right) const
		{
			assert(left .first != left .second);
			assert(right.first != right.second);

			return heap.comp_item_aux(right, left, is_stable_sort());
		}
	};
	friend class HeapCompare;

public:

	/**
	 @brief construct

	 @par 图示如下：
	 @code
	   [                 range                   ]
	 0 [first..............................second]
	 1 [first..............................second]
	 2 [first..............................second] <--- 每个序列均已按 comp 排序
	 3 [first..............................second]
	 4 [first..............................second]
	 5 [first..............................second]
	 @endcode

	 @param range[in,out] range 中的每个元素都是一个 pair<_InIt, _InIt>，表示一个递增子序列 [first, second)
	 @param comp    value 的比较器
	 */
	HeapMultiWay_TrivalWayIter(way_vec_t& range, Compare comp = Compare(), bool allowEmptyRange = true)
		: m_range(range), m_comp(comp)
	{
		m_old_size = range.size();
		if (allowEmptyRange)
			range.erase(std::remove_if(range.begin(), range.end(), FirstEqualSecond()), range.end());
		else
			assertNotEmpty();

		std::make_heap(range.begin(), range.end(), HeapCompare(*this));
	}

	bool is_end()
	{
		return m_range.empty();
	}

	const value_type& current_value() const
	{
		assert(!m_range.empty());
		return *m_range.front().first;
	}

	const WayIter& current_input() const
	{
		assert(!m_range.empty());
		return m_range.front();
	}

	int current_way() const
	{
		assert(!m_range.empty());
		return m_range.front().index;
	}

	void increment()
	{
		assert(!m_range.empty());
		++m_range.front().first;
		ajust_for_update_top();
	}

	void ajust_for_update_top()
	{
		if (FirstEqualSecond()(m_range.front())) {
			pop_heap_ignore_top(m_range.begin(), m_range.end(), HeapCompare(*this));
			m_range.pop_back();
		} else
			adjust_heap_top(m_range.begin(), m_range.end(), HeapCompare(*this));
	}

	WayIter& top()
	{
		assert(!m_range.empty());
		return m_range.front();
	}

private:
	way_vec_t&   m_range;
	size_t       m_old_size;
	Compare      m_comp;
	KeyExtractor m_key_extractor;

	inline bool comp_item_aux(const way_iter_t& x, const way_iter_t& y,
							  boost::mpl::true_ isStableSort) const
	{
		assert(x.first != x.second);
		assert(y.first != y.second);

		return comp_key_stable(x.index, y.index,
				*x.first, *y.first,
				typename HasTriCompare<Compare>::type());
	}
	bool comp_key_stable(int x, int y, const value_type& kx, const value_type& ky,
						 boost::mpl::true_ hasTriCompare) const
	{
		int ret = m_comp.compare(kx, ky);
		if (ret < 0)
			return true;
		if (ret > 0)
			return false;
		else
			return x < y;
	}
	bool comp_key_stable(int x, int y, const value_type& kx, const value_type& ky,
						 boost::mpl::false_ hasTriCompare) const
	{
		if (m_comp(kx, ky))
			return true;
		if (m_comp(ky, kx))
			return false;
		else
			return x < y;
	}

	inline bool comp_item_aux(const way_iter_t& x, const way_iter_t& y,
							  boost::mpl::false_ isStableSort) const
	{
		assert(x.first != x.second);
		assert(y.first != y.second);
		return m_comp(*x.first, *y.first);
	}

	size_t total_ways() const
	{
		return m_old_size;
	}

	bool is_any_way_end() const
	{
		return m_range.size() != m_old_size;
	}

	void assertNotEmpty()
	{
		for (int i = 0; i != m_range.size(); ++i)
		{
			assert(m_range[i].first != m_range[i].second);
		}
	}
};

#define HP_way_iter_traits typename std::iterator_traits<typename std::iterator_traits<RandIterOfInput>::value_type::first_type>

template< class RandIterOfInput

		, class KeyType = HP_way_iter_traits::value_type

		, bool StableSort = false //!< same Key in various way will output by way order

		, class Compare = std::less<KeyType>

		, class KeyExtractor = typename boost::mpl::if_c<
				boost::is_same<KeyType,
							   HP_way_iter_traits::value_type
							  >::value,
				boost::multi_index::identity<KeyType>,
				MustProvideKeyExtractor
			>::type

	    , CacheLevel HowCache = cache_default
		>
class HeapMultiWay :
	public CacheStrategy < typename std::iterator_traits<RandIterOfInput>::value_type::first_type
						 , KeyType
						 , KeyExtractor
						 , HowCache
						 >,
	public MultiWay_SetOP< HP_way_iter_traits::value_type
						 , KeyType
						 , HeapMultiWay<RandIterOfInput, KeyType, StableSort, Compare, KeyExtractor, HowCache>
						 >
{
	DECLARE_NONE_COPYABLE_CLASS(HeapMultiWay)

	typedef CacheStrategy< typename std::iterator_traits<RandIterOfInput>::value_type::first_type
						 , KeyType
						 , KeyExtractor
						 , HowCache
						 >
	super;

	friend class MultiWay_SetOP<HP_way_iter_traits::value_type
						 , KeyType
						 , HeapMultiWay<RandIterOfInput, KeyType, StableSort, Compare, KeyExtractor, HowCache>
						 >;
public:
	typedef typename std::iterator_traits<RandIterOfInput>::value_type way_pair_t; // this is std::pair<InIt,InIt>
	typedef typename way_pair_t::first_type way_iter_t;
	typedef HP_way_iter_traits::value_type  value_type;
	typedef KeyType      key_type;
	typedef KeyExtractor key_extractor;

	typedef boost::integral_constant<bool, StableSort> is_stable_sort;

	typedef typename super::cache_category cache_category;

	class HeapCompare
	{
		const HeapMultiWay& heap;
	public:
		explicit HeapCompare(const HeapMultiWay& heap) : heap(heap) {}

		//! 反转输入参数，用做堆操作的比较
		inline bool operator()(int x, int y) const
		{
			assert(heap.m_beg[x].first != heap.m_beg[x].second);
			assert(heap.m_beg[y].first != heap.m_beg[y].second);

			return heap.comp_cache_item(y, x, cache_category(), is_stable_sort());
		}
	};
	friend class HeapCompare;

public:
	/**
	 @brief construct

	 @par 图示如下：
	 @code

RandIterOfInput{pair}    first                                  second
      ||                   ||                                     ||
	  ||                   ||                                     ||
	  \/                   \/                                     \/
	 first--> 0 way_pair_t [min_value..............................)
	   /      1 way_pair_t [min_value..............................) <--- 每个序列均已
	   |      2 way_pair_t [min_value..............................)      按 comp 排序
	   |      3 way_pair_t [min_value..............................)
	  <       4 way_pair_t [min_value..............................)
	   |      5 way_pair_t [min_value..............................)
	   |      7 way_pair_t [min_value..............................)
	   \      8 way_pair_t [min_value..............................)
	 last---> end

	 @endcode

	 @param first, last [in,out] 每个元素都是一个 way_pair_t, 表示一个递增子序列 [first, second)
	 @param comp    value 的比较器

	 @note
	  - 一般情况下，这个类的性能不如 LoserTree, 但是，这个类也有一些优点：
	    - 对每个序列用 make_pair(first, last) 来表示，概念上更清晰
	    - 序列最后不需要一个 max_value 结束标志，这一点上胜于 LoserTree
		  - 当无法提供 max_value 时，使用这个类也许是更好的办法，比如当 int key 在整个 int 值域内都有定义时
	  - LoserTree 的每个序列只用一个 iterator 表示，序列结束用 MAX_KEY 做标志
	 */
	HeapMultiWay(RandIterOfInput first, RandIterOfInput last,
				 const Compare& comp = Compare(),
				 const KeyExtractor& keyExtractor = KeyExtractor())
	{
		init(first, last, comp, keyExtractor);
	}
	HeapMultiWay(RandIterOfInput first, int length,
				 const Compare& comp = Compare(),
				 const KeyExtractor& keyExtractor = KeyExtractor())
	{
		init(first, first + length, comp, keyExtractor);
	}

	HeapMultiWay()
	{
	}

	void init(RandIterOfInput first, RandIterOfInput last,
			  const Compare& comp = Compare(),
			  const KeyExtractor& keyExtractor = KeyExtractor())
	{
		assert(first < last); // ensure that will not construct empty loser tree

		m_old_size = last - first;

		this->m_comp = comp;
		this->m_key_extractor = keyExtractor;

		m_beg = first;
		m_end = last;

		m_heap.reserve(m_old_size);
		this->resize_cache(m_old_size);

		for (RandIterOfInput i = first; i != last; ++i)
		{
			if (i->first != i->second)
			{
				int way_idx = i - first;
				m_heap.push_back(way_idx);
				// read first value from every sequence
				this->input_cache_item(way_idx, i->first);
			}
		}
		std::make_heap(m_heap.begin(), m_heap.end(), HeapCompare(*this));
	}

	bool is_end() const
	{
		return m_heap.empty();
	}

	const value_type& current_value() const
	{
		assert(!m_heap.empty());
		return current_value_aux(cache_category());
	}

	int current_way() const
	{
		assert(!m_heap.empty());
		return m_heap[0];
	}

	void increment()
	{
		assert(!m_heap.empty());
		int top_way = m_heap[0];
		way_pair_t& top = *(m_beg + top_way);
		++top.first;
		if (febird_unlikely(top.first == top.second)) {
			pop_heap_ignore_top(m_heap.begin(), m_heap.end(), HeapCompare(*this));
			m_heap.pop_back();
		} else {
			this->input_cache_item(top_way, top.first);
			adjust_heap_top(m_heap.begin(), m_heap.end(), HeapCompare(*this));
		}
	}

	void ajust_for_update_top()
	{
		int top_way = m_heap[0];
		way_pair_t& top = *(m_beg + top_way);
	//	++top.first; // caller may inc multi for top.first
		if (febird_unlikely(top.first == top.second)) {
			pop_heap_ignore_top(m_heap.begin(), m_heap.end(), HeapCompare(*this));
			m_heap.pop_back();
		} else {
			this->input_cache_item(top_way, top.first);
			adjust_heap_top(m_heap.begin(), m_heap.end(), HeapCompare(*this));
		}
	}

	way_pair_t& top()
	{
		return *(m_beg + m_heap[0]);
	}

protected:
	const value_type& current_value_aux(tag_cache_none) const
	{
		assert(m_heap[0] < m_end - m_beg);
		return *(*(m_beg + m_heap[0])).first;
	}
	const value_type& current_value_aux(tag_cache_key) const
	{
		assert(m_heap[0] < m_end - m_beg);
		return *(*(m_beg + m_heap[0])).first;
	}
	const value_type& current_value_aux(tag_cache_value) const
	{
		assert(m_heap[0] < m_end - m_beg);
		return this->m_cache[m_heap[0]];
	}

	using super::get_cache_key;
	inline const KeyType get_cache_key(int nth, tag_cache_none) const
	{
		assert(m_beg + nth < m_end);
		const way_pair_t& p = *(m_beg + nth);
		assert(p.first != p.second);
		return this->m_key_extractor(*p.first);
	}

	template<class CacheCategory>
	inline bool comp_cache_item(int x, int y,
								CacheCategory cache_tag,
								boost::mpl::true_ isStableSort) const
	{
		return comp_key_stable(x, y,
				get_cache_key(x, cache_tag),
				get_cache_key(y, cache_tag),
				typename HasTriCompare<Compare>::type());
	}
	bool comp_key_stable(int x, int y, const KeyType& kx, const KeyType& ky,
						 boost::mpl::true_ hasTriCompare) const
	{
		int ret = m_comp.compare(kx, ky);
		if (ret < 0)
			return true;
		if (ret > 0)
			return false;
		else
			return x < y;
	}
	bool comp_key_stable(int x, int y, const KeyType& kx, const KeyType& ky,
						 boost::mpl::false_ hasTriCompare) const
	{
		if (m_comp(kx, ky))
			return true;
		if (m_comp(ky, kx))
			return false;
		else
			return x < y;
	}

	template<class CacheCategory>
	inline bool comp_cache_item(int x, int y,
								CacheCategory cache_tag,
								boost::mpl::false_ isStableSort) const
	{
		return m_comp(get_cache_key(x, cache_tag), get_cache_key(y, cache_tag));
	}

	void assertNotEmpty(RandIterOfInput first, RandIterOfInput last) const
	{
		for (RandIterOfInput i = first; i != last; ++i)
		{
			assert((*i).first != (*i).second);
		}
	}
	bool is_any_way_end() const
	{
		return m_heap.size() != m_old_size;
	}

	size_t total_ways() const { return m_old_size; }

protected:
	RandIterOfInput  m_beg;
	RandIterOfInput  m_end;
	std::vector<int> m_heap;
	size_t			 m_old_size;
	Compare		     m_comp;
};


//! TODO:

template<class InputIterator>
class InputIteratorReader
{
	InputIterator m_end;
public:
	bool is_end(const InputIterator& iter) const { return iter != m_end; }
};

/**
 @brief 当相同元素的数目超过 minDup 时，将元素拷贝到输出

 这是一个很有用的 multi_way_copy_if._Cond, 并且也作为一个编写 _Cond 的示例
 可以在 MultiWay_CopyAtLeastDup 中对 value 进行某方面的统计（例如记录每个 value 的重复次数）
 */
class MultiWay_CopyAtLeastDup
{
	int m_minDup;

public:
	MultiWay_CopyAtLeastDup(int minDup) : m_minDup(minDup) {}

	template<class ValueT>
	bool operator()(int   equal_count,
					const ValueT& prev_value,
					const ValueT& curr_value) const
	{
		// 仅当 equal_count 第一次达到 m_minDup 时才返回 true
		// 返回 equal_count >= m_minDup 会导致多次拷贝

		return equal_count == m_minDup;
	}
};

template<class _Container>
class table_insert_iterator :
	public std::iterator<std::output_iterator_tag, typename _Container::value_type>
{
public:
	typedef _Container container_type;
	typedef typename _Container::reference reference;

//	typedef _Range_checked_iterator_tag _Checked_iterator_category;

	explicit table_insert_iterator(_Container& _Cont)
		: container(&_Cont)
	{	// construct with container
	}

	template<class KeyValue>
	table_insert_iterator<_Container>& operator=(const KeyValue& kv)
	{	// insert (key, value) into container
		container->insert(kv);
		return (*this);
	}

	table_insert_iterator<_Container>& operator*()
	{	// pretend to return designated value
		return (*this);
	}

	table_insert_iterator<_Container>& operator++()
	{	// pretend to preincrement
		return (*this);
	}

	table_insert_iterator<_Container> operator++(int)
	{	// pretend to postincrement
		return (*this);
	}

protected:
	_Container *container;	// pointer to container
};

template<class _Container>
table_insert_iterator<_Container>
table_inserter(_Container& cont)
{
	return table_insert_iterator<_Container>(cont);
}

} // namespace multi_way

} // namespace febird

#endif // __febird_set_op_h__

// @} end file set_op.hpp


