
#include <time.h>

#include <glog/logging.h>
#include <gflags/gflags.h>

#include "TimeWait.h"
#include "libevent_wrapper.h"

DEFINE_int32(WAITER_threads, 5, "");
DEFINE_int32(WAITER_usleep, 10, "");
DEFINE_int32(WAITER_nores_usleep, 100, "");
DEFINE_int32(WAITER_rqst_queue_size, 5000, "");
DEFINE_int32(WAITER_rslt_queue_size, 6000, "");


namespace spider {
  namespace download {

class _TimeWait_
{
    struct _InnerArgs_ {
        void* args;
        _TimeWait_* self;
        struct event timer_ev;
        struct event* timer_ev2;
        void* rqst;
    };
public:
    _TimeWait_(struct event_base* base)
      : base_(base)
    {}

    void operator()(void* args) {
        _process_(args);
    }

    void _process_(void* args) {

        TimeWait* waiter = (TimeWait*)args;
        assert(waiter);


        while(waiter->is_running_) {
            TimeWait::_Args_* rqst = waiter->wait_list_.pop();
            if(rqst == NULL) {
                usleep(FLAGS_WAITER_nores_usleep);
                continue;
            }

            if(rqst->wait_ms > 0) {
                _InnerArgs_* _inargs_ = new _InnerArgs_;
                _inargs_->args = args;
                _inargs_->self = this;
                _inargs_->rqst = (void*)rqst;
                _inargs_->timer_ev2 = new struct event;
                int tv_sec = rqst->wait_ms/1000; 
                int tv_usec = (rqst->wait_ms%1000)*1000;
                //LOG(INFO)<<"TIMEWAIT add timer event "<<tv_sec<<":"<<tv_usec;
                struct timeval tv;
                evutil_timerclear(&tv);
                tv.tv_sec = rqst->wait_ms/1000; tv.tv_usec = (rqst->wait_ms%1000)*1000;
                timeout_set(_inargs_->timer_ev2, _TimeWait_::_ready_rqst_handler_, (void*)_inargs_);
                event_base_set(base_, _inargs_->timer_ev2);
                timeout_add(_inargs_->timer_ev2, &tv);

            } else {
                // 直接放入ready队列
                waiter->ready_list_.push(rqst->userdata);
                delete rqst; rqst = NULL;
            }
            usleep(FLAGS_WAITER_usleep);
        }

        return;
    }

    static void _ready_rqst_handler_(const int fd, const short which, void *args) 
    {
        _InnerArgs_* _inargs_ = (_InnerArgs_*)args;
        TimeWait* waiter = (TimeWait*)_inargs_->args;
        _TimeWait_* self = (_TimeWait_*)_inargs_->self;
        TimeWait::_Args_* rqst = (TimeWait::_Args_*)_inargs_->rqst;
        // 放入ready队列
        waiter->ready_list_.push(rqst->userdata);
        delete _inargs_; _inargs_ = NULL;
        delete rqst; rqst = NULL;
    }

private:
    struct event_base* base_;
};

class _TimeWaitReactor_
{
public:
    void operator()(void* args)
    {
        _start_threads(args);
    }

    void* _start_threads(void* args)
    {
        TimeWait* waiter = (TimeWait*)args;
        assert(waiter);

        base_ = event_base_new();

        // 添加timer，避免无抓取任务时event_base_dispatch循环退出
        my_add_timer(base_, &clockevent_, _TimeWaitReactor_::_clock_handler, 1, 0, this);

        threadpool threadpool_;
        threadpool_.create_thread(_TimeWait_(base_), (void*)waiter);

        event_base_dispatch(base_); 

    }

    static void _clock_handler(const int fd, const short which, void *args) 
    {
        _TimeWaitReactor_* self = (_TimeWaitReactor_*)args;
        my_add_timer(self->base_, &self->clockevent_, _TimeWaitReactor_::_clock_handler, 10, 0, args);
    }

private:
    struct event_base* base_;
    struct event clockevent_;
};


TimeWait::TimeWait()
  : wait_list_(FLAGS_WAITER_rqst_queue_size), ready_list_(FLAGS_WAITER_rslt_queue_size), is_running_(false)
{
}

TimeWait::~TimeWait()
{
    stop();
}

void TimeWait::start()
{
    is_running_ = true;
    LOG(INFO)<<"TIMEWAIT start "<<FLAGS_WAITER_threads<<" threads";
    for(int i=0; i<FLAGS_WAITER_threads; i++) {
        threadpool_.create_thread(_TimeWaitReactor_(), (void*)this);
    }
}

void TimeWait::stop()
{
    is_running_ = false;
    threadpool_.wait_all();
    LOG(INFO)<<"TIMEWAIT stop "<<FLAGS_WAITER_threads<<" threads";
}

int TimeWait::add(int wait_ms, void* userdata)
{
    _Args_* args = new _Args_;
    args->wait_ms = wait_ms;
    args->userdata = userdata;
    wait_list_.push_wait(args);
    return 0;
}

void* TimeWait::getReady()
{
    return ready_list_.pop();
}

}
}

