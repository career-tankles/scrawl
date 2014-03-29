#ifndef __BOOST_THREAD_POOL_H__
#define __BOOST_THREAD_POOL_H__

#include <iostream>
#include <boost/bind.hpp>
#include <boost/thread/thread.hpp>

struct _Task {
    void operator()(void* args) {
        std::cerr<<"this is only one test!"<<std::endl;
    }
};

class threadpool 
{
public:
    threadpool() {}
    ~threadpool() {
		wait_all();
	}
    
    template<typename __Task>
    void create_thread(__Task handler, void* args)
    {   
        _threads.create_thread(boost::bind<void>(handler, args));
    }
    void wait_all() {
        _threads.join_all();
    }
    
private:
    boost::thread_group _threads;

};    

/**
void run(void* args) {
    std::cerr<<"this is only one test!"<<std::endl;
}
_threads.create_thread(&run, argss);

**/


#endif

