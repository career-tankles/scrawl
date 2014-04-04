
#include <time.h>

#include <glog/logging.h>
#include <gflags/gflags.h>

#include "TimeWait.h"
#include "conf.h"
#include "libevent_wrapper.h"


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
            TimeWait::_Args_* wait_rqst = waiter->wait_list_.pop();
            if(wait_rqst == NULL) {
                usleep(FLAGS_WAITER_nores_usleep);
                continue;
            }

            if(wait_rqst->wait_ms > 0) {
                _InnerArgs_* _inargs_ = new _InnerArgs_;
                _inargs_->args = args;
                _inargs_->self = this;
                _inargs_->rqst = (void*)wait_rqst;
                //_inargs_->timer_ev2 = new struct event;
                struct timeval tv;
                evutil_timerclear(&tv);
                tv.tv_sec = wait_rqst->wait_ms/1000; tv.tv_usec = (wait_rqst->wait_ms%1000)*1000;
                timeout_set(&_inargs_->timer_ev, _TimeWait_::_ready_rqst_handler_, (void*)_inargs_);
                event_base_set(base_, &_inargs_->timer_ev);
                timeout_add(&_inargs_->timer_ev, &tv);

            } else {
                // 直接放入ready队列
                waiter->ready_list_.push(wait_rqst->userdata);
                delete wait_rqst; wait_rqst = NULL;
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
        TimeWait::_Args_* wait_rqst = (TimeWait::_Args_*)_inargs_->rqst;
        // 放入ready队列
        waiter->ready_list_.push(wait_rqst->userdata);

        timeout_del(&_inargs_->timer_ev);
        delete wait_rqst; wait_rqst = NULL;
        delete _inargs_; _inargs_ = NULL;
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
        my_add_timer(self->base_, &self->clockevent_, _TimeWaitReactor_::_clock_handler, 0, 1000, args);
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

