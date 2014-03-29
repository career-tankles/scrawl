#ifndef __CONCURRENT_QUEUE_H__
#define __CONCURRENT_QUEUE_H__

#include <stdio.h>

#include <queue>
#include <iostream>

#include <boost/thread/mutex.hpp>
#include <boost/thread/condition.hpp>


template<typename TYPE>
class simple_queue
{
public:
    simple_queue(long max_size)
        : _max_size(max_size)
    {
    }
    ~simple_queue()
    {
        std::cerr<<"exit with "<<_queue.size()<<" items!!!"<<std::endl;
    }
    bool push(TYPE const& data)
    {
        boost::mutex::scoped_lock lock(_mutex);
        if(_max_size > 0 && _queue.size() >= _max_size)
            return false;

        _queue.push(data);
        lock.unlock();
        _not_empty_cond.notify_one();
        return true;
    }

    void push_wait(TYPE const& data)
    {
        boost::mutex::scoped_lock lock(_mutex);
        while(_max_size > 0 && _queue.size() >= _max_size)
            _not_full_cond.wait(lock);

        _queue.push(data);
        lock.unlock();
        _not_empty_cond.notify_one();
    }

    bool empty() const
    {
        boost::mutex::scoped_lock lock(_mutex);
        return _queue.empty();
    }

    bool pop(TYPE& val)
    {
        boost::mutex::scoped_lock lock(_mutex);
        if(_queue.empty())
            return false;
        val = _queue.front();
        _queue.pop();
        lock.unlock();
        _not_full_cond.notify_one();
        return true;
    }

    void wait_pop(TYPE& val)
    {
        boost::mutex::scoped_lock lock(_mutex);
        while(_queue.empty())
            _not_empty_cond.wait(lock);
        
        val = _queue.front();
        _queue.pop();
        lock.unlock();
        _not_full_cond.notify_one();
    }

private:
    long _max_size;
    std::queue<TYPE> _queue;
    mutable boost::mutex _mutex;
    boost::condition_variable _not_full_cond;
    boost::condition_variable _not_empty_cond;
};

template<typename TYPE>
class ptr_queue
{
public:
    ptr_queue(size_t max_size)
        : _max_size(max_size)
    {
    }
    ~ptr_queue()
    {
        std::cerr<<"exit with "<<_queue.size()<<" items!!!"<<std::endl;
    }

    bool push(TYPE* data)
    {
        boost::mutex::scoped_lock lock(_mutex);
        if(_max_size > 0 && _queue.size() >= _max_size)
            return false;

        _queue.push(data);
        lock.unlock();
        _not_empty_cond.notify_one();
        return true;
    }

    void push_wait(TYPE* data)
    {
        boost::mutex::scoped_lock lock(_mutex);
        while(_max_size > 0 && _queue.size() >= _max_size)
            _not_full_cond.wait(lock);

        _queue.push(data);
        lock.unlock();
        _not_empty_cond.notify_one();
    }


    TYPE* pop()
    {
        boost::mutex::scoped_lock lock(_mutex);
        if(_queue.empty())
            return NULL;
        TYPE* val = _queue.front();
        _queue.pop();
        lock.unlock();
        _not_full_cond.notify_one();
        return val;
    }

    TYPE* wait_pop()
    {
        boost::mutex::scoped_lock lock(_mutex);
        while(_queue.empty())
            _not_empty_cond.wait(lock);
        
        TYPE* val = _queue.front();
        _queue.pop();
        lock.unlock();
        _not_full_cond.notify_one();
    }

    bool empty() const
    {
        boost::mutex::scoped_lock lock(_mutex);
        return _queue.empty();
    }
    size_t size() const {
        return _queue.size();
    }
    size_t capacity() const {
        return _max_size;
    }

private:
    size_t _max_size;
    std::queue<TYPE*> _queue;
    mutable boost::mutex _mutex;
    boost::condition_variable _not_full_cond;
    boost::condition_variable _not_empty_cond;
};

#endif

