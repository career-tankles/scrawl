#ifndef __TIME_WAIT_H__
#define __TIME_WAIT_H__

#include <event.h>

#include "simple_queue.h"
#include "threadpool.h"

namespace spider {
namespace download {

class TimeWait
{
public:
    TimeWait();
    ~TimeWait();

    void start();
    void stop();

    int add(int wait_ms, void* userdata=NULL);
    void* getReady();

private:
    struct _Args_ {
        int wait_ms;        // 要等待的毫秒
        void* userdata;     // 用户数据
    };

    ptr_queue<_Args_> wait_list_;
    ptr_queue<void> ready_list_;
    threadpool threadpool_;
    bool is_running_;

    friend class _TimeWaitReactor_;
    friend class _TimeWait_;

};

}

}

#endif

