#ifndef __STORE_H__
#define __STORE_H__

#include <string>

#include "threadpool.h"
#include "simple_queue.h"
#include "message.h"

using namespace ::spider::message;

namespace spider {
namespace store {

class PageStorage
{
public:
    PageStorage();
    ~PageStorage();
    
    int start();
    int stop();
    int submit(struct http_result_t* result);

private:
    bool is_running_;
    threadpool threadpool_;
    ptr_queue<struct http_result_t>  http_result_;
    
    friend class _PageStorage_;
};

}

}

#endif

