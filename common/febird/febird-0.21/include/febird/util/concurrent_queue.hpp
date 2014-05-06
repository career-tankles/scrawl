/* vim: set tabstop=4 : */
#ifndef __febird_concurrent_queue_h__
#define __febird_concurrent_queue_h__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
# pragma once
# pragma warning(push)
# pragma warning(disable: 4018)
#endif

#include "../stdtypes.hpp"
#include <boost/thread/mutex.hpp>
#include <boost/thread/condition.hpp>

namespace febird { namespace util {

/**
 @ingroup util
 @brief 并发队列
  
  - 一般用于多线程中的生产者-消费者模型

  - 生产者线程向队列中放入数据 @see push_back, insert, insert_unique

  - 消费者线程从队列中取出数据 @see pop_front, remove

 @param Container 使用的容器

 @note support all kinds of stl container!
 @note but, std::vector is low performance when used it...
 @note 使用 map/set 时，放入数据用 insert_unique
 @note 使用 multimap/multiset 时，放入数据用 insert
 */
template<class Container>
class concurrent_queue
{
	DECLARE_NONE_COPYABLE_CLASS(concurrent_queue)

	Container m_queue;
	boost::mutex  m_mtx;
	boost::condition m_cond;
	size_t m_maxSize;

public:
	typedef Container					   queue_type;
	typedef typename Container::size_type  size_type;
	typedef typename Container::value_type value_type;

public:
	//! default will not limit max size
	explicit concurrent_queue(int maxSize = INT_MAX) : m_maxSize(maxSize) {}

	//! not needed lock...
	int maxSize() const throw()	{ return m_maxSize;	}

	//! locked get size...
	size_type size()
	{
		boost::mutex::scoped_lock lock(m_mtx);
		return m_queue.size();
	}

	//! locked test
	bool empty()
	{
		boost::mutex::scoped_lock lock(m_mtx);
		return m_queue.empty();
	}

	//! locked test
	bool full()
	{
		boost::mutex::scoped_lock lock(m_mtx);
		return m_queue.size() == m_maxSize;
	}

	/**
	 @brief 删除队列中的所有元素
	 
	 @note
		this function will cause the concurrent_queue to a non-consistent state
		because m_queue.size() will    not equal to the value of m_semForGet,
		and m_maxSize - m_queue.size() not equal to the value of m_semForPut

		be sure after call this function,
		you will not call any push/pop/insert/remove function on this concurrent_queue
	 */
	void clearQueue()
	{
		boost::mutex::scoped_lock lock(m_mtx);
		m_queue.clear();
	}

	//@{
	//! not locked...
	size_type peekSize() const throw() { return m_queue.size();	}
	bool peekEmpty() const throw() { return m_queue.empty(); }
	bool peekFull()  const throw() { return m_queue.size() == m_maxSize; }
	//@}

	//@{
	// low level operations:
	queue_type& queue()  throw() { return m_queue; }
	//@}

	struct is_not_empty {
		Container& cq;
		explicit is_not_empty(Container& q) : cq(q) {}
		bool operator()() const { return !cq.empty(); }
	};
	struct is_not_full {
		Container& cq;
		size_t max_size;
		explicit is_not_full(Container& q, size_t n) : cq(q), max_size(n) {}
		bool operator()() const { return cq.size() < max_size; }
	};
public:

	/**
	 @name push/pop functions
	 @brief used for queue/priority_queue/stack

	 @note
		similar with push/pop functions,
		but the 'pop' and 'gettop' are combined as an atom operation
	 @{
	 */
	void push(const value_type& value)
	{
		boost::mutex::scoped_lock lock(m_mtx);
		while (m_queue.size() >= m_maxSize) m_cond.wait(lock);
		assert(m_queue.size() <  m_maxSize);
		m_queue.push(value); // require this method
		m_cond.notify_all();
	}
	void pop(value_type& result)
	{
		boost::mutex::scoped_lock lock(m_mtx);
		while (m_queue.empty()) m_cond.wait(lock);
		assert(!m_queue.empty());
		result = m_queue.front(); // front, not back()!!
		m_queue.pop(); // require this method
		m_cond.notify_all();
	}
	bool push(const value_type& value, int timeout)
	{
		boost::mutex::scoped_lock lock(m_mtx);
		if (!m_cond.timed_wait(lock, boost::posix_time::milliseconds(timeout), is_not_full(m_queue, m_maxSize))) return false;
		assert(m_queue.size() < m_maxSize);
		m_queue.push(value); // require this method
		m_cond.notify_all();
		return true;
	}
	bool pop(value_type& result, int timeout)
	{
		boost::mutex::scoped_lock lock(m_mtx);
		if (!m_cond.timed_wait(lock, boost::posix_time::milliseconds(timeout), is_not_empty(m_queue))) return false;
		assert(!m_queue.empty());
		result = m_queue.front();
		m_queue.pop(); // require this method
		m_cond.notify_all();
		return true;
	}
	value_type pop()
	{
		value_type value;
		pop(value);
		return value;
	}
	//@} push/front end...
	//////////////////////////////////////////////////////////////////////

	//////////////////////////////////////////////////////////////////////
	/**
	 @name push_back/pop_front/push_front/pop_back methods
	  
	// used for list/vector/deque
	// slist(stlport.slist) can only use push_back/pop_front
	//
	// @note
	//   similar with Container's functions,
	//   but the 'pop_front/pop_back' and 'front/back'
	//   are combined as an atom operation
	 @{
	 */ 
	void push_back(const value_type& value)
	{
		boost::mutex::scoped_lock lock(m_mtx);
		while (m_queue.size() >= m_maxSize) m_cond.wait(lock);
		assert(m_queue.size() < m_maxSize);
		m_queue.push_back(value); // require this method
		m_cond.notify_all();
	}
	void pop_front(value_type& result) 
	{
		boost::mutex::scoped_lock lock(m_mtx);
		while (m_queue.empty()) m_cond.wait(lock);
		assert(!m_queue.empty());
		result = m_queue.front();
		m_queue.pop_front(); // require this method
		m_cond.notify_all();
	}
	bool push_back(const value_type& value, int timeout) 
	{
		boost::mutex::scoped_lock lock(m_mtx);
		if (!m_cond.timed_wait(lock, boost::posix_time::milliseconds(timeout), is_not_full(m_queue, m_maxSize))) return false;
		assert(m_queue.size() < m_maxSize);
		m_queue.push_back(value); // require this method
		m_cond.notify_all();
		return true;
	}
	bool pop_front(value_type& result, int timeout)  
	{
		boost::mutex::scoped_lock lock(m_mtx);
		if (!m_cond.timed_wait(lock, boost::posix_time::milliseconds(timeout), is_not_empty(m_queue))) return false;
		assert(!m_queue.empty());
		result = m_queue.front();
		m_queue.pop_front(); // require this method
		m_cond.notify_all();
		return true;
	}
	value_type pop_front()  
	{
		value_type value;
		pop_front(value);
		return value;
	}

	//@{
	//! push_front/pop_back...
	void push_front(const value_type& value) 
	{
		boost::mutex::scoped_lock lock(m_mtx);
		while (m_queue.size() >= m_maxSize) m_cond.wait(lock);
		assert(m_queue.size() < m_maxSize);
		m_queue.push_front(value); // require this method
		m_cond.notify_all();
	}
	void pop_back(value_type& result)  
	{
		boost::mutex::scoped_lock lock(m_mtx);
		while (m_queue.empty()) m_cond.wait(lock);
		assert(!m_queue.empty());
		result = m_queue.back();
		m_queue.pop_back(); // require this method
		m_cond.notify_all();
	}
	bool push_front(const value_type& value, int timeout)
	{
		boost::mutex::scoped_lock lock(m_mtx);
		if (!m_cond.timed_wait(lock, boost::posix_time::milliseconds(timeout), is_not_full(m_queue, m_maxSize))) return false;
		assert(m_queue.size() < m_maxSize);
		m_queue.push_front(value); // require this method
		m_cond.notify_all();
		return true;
	}
	bool pop_back(value_type& result, int timeout)  
	{
		boost::mutex::scoped_lock lock(m_mtx);
		if (!m_cond.timed_wait(lock, boost::posix_time::milliseconds(timeout), is_not_empty(m_queue, m_maxSize))) return false;
		assert(!m_queue.empty());
		result = m_queue.back();
		m_queue.pop_back(); // require this method
		m_cond.notify_all();
		return true;
	}
	value_type pop_back()  
	{
		value_type value;
		pop_back(value);
		return value;
	}
	//@} end push_back/pop_front/push_front/pop_back end...
	//////////////////////////////////////////////////////////////////////

	//////////////////////////////////////////////////////////////////////
	//! @name for multiset/multimap.
	//@{

	bool exists(const value_type& value)
	{
		return exists<value_type>(value);
	}
	template<class KeyType>
	bool exists(const KeyType& key)
	{
		boost::mutex::scoped_lock lock(m_mtx);
		return (m_queue.find(key) != m_queue.end());
	}

	/**
	 @brief for multiset/multimap
	// @note
	//   if used for set/map, it will not compile.
	//   please use insert_unique() for set/map.
	 */
	typename Container::iterator insert(const value_type& value)  
	{
		boost::mutex::scoped_lock lock(m_mtx);
		m_cond.timed_wait(lock, is_not_full(m_queue, m_maxSize));
		assert(m_queue.size() < m_maxSize);
		typename Container::iterator iter = m_queue.insert(value); // require this method
		m_cond.notify_all();
		return iter;
	}

	/**
	 @brief for set/map
	  
	//   if Container contains a value same as 'value', return false
	//   else return true.
	 */
	bool insert_unique(const value_type& value)  
	{
		boost::mutex::scoped_lock lock(m_mtx);
		m_cond.timed_wait(lock, is_not_full(m_queue, m_maxSize));
		assert(m_queue.size() < m_maxSize);
		std::pair<typename Container::iterator, bool>
		   	res = m_queue.insert(value); // require this method
		// do not return res.first, it is the insert position.
		// because in concurrent case, the insert position is
		// not ensured valid in different lock area.
		m_cond.notify_all();
		return res.second;
	}

	/**
	 @brief 删除元素并返回
	  
	 @param[out] result 返回删除的元素
	 @param[in]  key    元素的键
	 @return 是否成功
		if return true,  value will be saved in 'result',
		if return false, 'result' will not be changed..
	 */
	template<class KeyType>
	size_t erase(const KeyType& key, value_type& result)  
	{
		boost::mutex::scoped_lock lock(m_mtx);
		return m_queue.erase(key);
	}

	//! when input key is saved in 'value'
	size_t erase(value_type& value)  
	{
		return erase<value_type>(value, value);
	}

private:
	template<class ResultContainer, class KeyType>
	bool remove_multi_one(ResultContainer& result, const KeyType& key, boost::system_time expire)  
	{
		boost::mutex::scoped_lock lock(m_mtx);
		if (!m_cond.timed_wait(lock, expire, is_not_empty(m_queue, m_maxSize))) return false;
		assert(!m_queue.empty());
		std::pair<typename Container::iterator, typename Container::iterator>
			ii = m_queue.equal_range(key); // require this method
		bool bRet = ii.first != ii.second;
		for (; ii.first != ii.second; ++ii.first) {
			result.push_back(*ii.first);
			m_queue.erase(ii.first); // require this method
		}
		m_cond.notify_all();
		return bRet;
	}
public:
	/**
	 @brief 删除多个[键值相同的]元素
	  
	 @note
		must ResultContainer must be empty when call this function...
		ResultContainer::value_type must assignable from m_queue.value_type
	 */
	template<class ResultContainer, class KeyType>
	void remove(ResultContainer& result, const KeyType& key, int timeout = 0)
	{
		boost::system_time const expire = boost::get_system_time() + boost::posix_time::milliseconds(timeout);
		assert(result.empty());
		while (remove_multi_one(result, key, expire))
			NULL;
	}

	//! @name low level functions, for any container
	//@{
	void remove_begin_elem(value_type& value)
	{
		boost::mutex::scoped_lock lock(m_mtx);
		while (m_queue.empty()) m_cond.wait(lock);
		assert(!m_queue.empty());
		value = *m_queue.begin();
		m_queue.erase(m_queue.begin());
		m_cond.notify_all();
	}
	void remove_end_elem(value_type& value)  
	{
		boost::mutex::scoped_lock lock(m_mtx);
		while (m_queue.empty()) m_cond.wait(lock);
		assert(!m_queue.empty());
		typename Container::iterator iter = m_queue.end();
		value = *--iter;
		m_queue.erase(iter);
		m_cond.notify_all();
	}
	value_type remove_begin_elem()
	{
		value_type value;
		remove_begin_elem(value);
		return value;
	}
	value_type remove_end_elem()
	{
		value_type value;
		remove_end_elem(value);
		return value;
	}
	//@}
};

} } // namespace febird::util

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
# pragma warning(pop)
#endif

#endif // __febird_concurrent_queue_h__
