
#include <time.h>

#include <glog/logging.h>
#include <gflags/gflags.h>

#include "TimeWait.h"
#include "conf.h"
#include "libevent_wrapper.h"


namespace spider {
  namespace download {

class _TimeWaitReactor_
{
    struct _InnerArgs_ {
        struct event_base* base;
        struct event timer_ev;
        struct timeval tv;
        void* args;
        void* data;
    
        _InnerArgs_(): base(NULL), args(NULL), data(NULL) {
            evutil_timerclear(&tv);
            tv.tv_sec = 1; tv.tv_usec = 0;
        }
    };

public:
    void operator()(void* args)
    {
        _start_threads(args);
    }

    void* _start_threads(void* args)
    {
        struct event_base* base_ = event_base_new();

        _InnerArgs_ clock_args;
        clock_args.base = base_;
        clock_args.args = args;
        clock_args.tv.tv_sec = 0;
        clock_args.tv.tv_usec = 1000;

        // 添加timer
        evtimer_set(&clock_args.timer_ev, _wait_rqst_handler_, (void*)&clock_args);
        event_base_set(base_, &clock_args.timer_ev);
        evtimer_add(&clock_args.timer_ev, &clock_args.tv);

        event_base_dispatch(base_); 
        std::cerr<<"after event_base_dispatch"<<std::endl;

        event_base_free(base_);

    }

    static void _wait_rqst_handler_(const int fd, const short which, void* args) {

        _InnerArgs_* clock_args = (_InnerArgs_*)args;

        TimeWait* waiter = (TimeWait*)clock_args->args;
        assert(waiter);

        const int max_slot = 10;
        int n = 0;
        while(waiter->is_running_ && n++ < max_slot) {
            TimeWait::_Args_* wait_rqst = waiter->wait_list_.pop();
            if(wait_rqst == NULL) {
                break;
            }

            if(wait_rqst->wait_ms > 0) {
                _InnerArgs_* _inargs_ = new _InnerArgs_;
                _inargs_->base = clock_args->base;
                _inargs_->args = clock_args->args;
                _inargs_->data = (void*)wait_rqst;
                _inargs_->tv.tv_sec = wait_rqst->wait_ms/1000; 
                _inargs_->tv.tv_usec = (wait_rqst->wait_ms%1000)*1000;

                timeout_set(&_inargs_->timer_ev, _ready_rqst_handler_, (void*)_inargs_);
                event_base_set(_inargs_->base, &_inargs_->timer_ev);
                timeout_add(&_inargs_->timer_ev, &_inargs_->tv);

                LOG(INFO)<<"TIMEWAIT timeout_add "<<_inargs_->tv.tv_sec<<" "<<_inargs_->tv.tv_usec;

            } else {
                // 直接放入ready队列
                waiter->ready_list_.push(wait_rqst->userdata);
                delete wait_rqst; wait_rqst = NULL;
            }
        }

        evtimer_add(&clock_args->timer_ev, &clock_args->tv);
    }

    static void _ready_rqst_handler_(const int fd, const short which, void *args) 
    {
        _InnerArgs_* _inargs_ = (_InnerArgs_*)args;
        TimeWait* waiter = (TimeWait*)_inargs_->args;
        TimeWait::_Args_* wait_rqst = (TimeWait::_Args_*)_inargs_->data;
        // 放入ready队列
        waiter->ready_list_.push(wait_rqst->userdata);
        LOG(INFO)<<"TIMEWAIT timeout ready ";

        timeout_del(&_inargs_->timer_ev);
        delete wait_rqst; wait_rqst = NULL;
        delete _inargs_; _inargs_ = NULL;
    }
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

