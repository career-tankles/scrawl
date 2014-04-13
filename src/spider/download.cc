
#include <time.h>
#include <glog/logging.h>
#include <gflags/gflags.h>

#include "conf.h"
#include "conn.h"
#include "download.h"
#include "libevent_wrapper.h"

namespace spider {
namespace download {

class _DownloaderReactor_
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
        clock_args.tv.tv_sec = FLAGS_DOWN_clock_interval_sec;
        clock_args.tv.tv_usec = FLAGS_DOWN_clock_interval_us;

        // 添加timer
        evtimer_set(&clock_args.timer_ev, _download_handler_, (void*)&clock_args);
        event_base_set(base_, &clock_args.timer_ev);
        evtimer_add(&clock_args.timer_ev, &clock_args.tv);

        event_base_dispatch(base_); 
        event_base_free(base_);
    }
    static void _download_handler_(const int fd, const short which, void* args) {

        _InnerArgs_* clock_args = (_InnerArgs_*)args;
        Downloader* downloader = (Downloader*)clock_args->args;
        assert(downloader);

        const int max_slot = 10;
        int n = 0;
        while(downloader->is_running_ && n++ < max_slot) {
            http_request_t* rqst = downloader->http_request_.pop();
            if(rqst == NULL) {
                break;
            }
            assert(!rqst->ip.empty() 
                    && !rqst->http_request_data.empty() 
                    && !rqst->url.empty());
            // 启动下载
            conn* c = conn_new(rqst->ip.c_str(), rqst->port);
            assert(c);
            c->user_data = (void*)rqst;                     // 记录请求
            c->user_data2 = (void*)downloader;
            c->user_data3 = (void*)clock_args->base;
            c->cb_result_handler_ = _result_handler_;       // 结果处理回调函数
            c->send_buf_rptr_ = (char*)rqst->http_request_data.c_str();
            c->send_buf_left_ = rqst->http_request_data.size();
            int ret = conn_connect(c);
            if(ret != 0 && errno != EINPROGRESS) {
                // connect 出错
                LOG(INFO)<<"DOWNLOAD "<< rqst->url<<" connect to "<<rqst->ip.c_str()<<":"<<rqst->port<<" failed";
                //c->sock_fd_ = -1;
                struct timeval tv = my_timeval(0, 1000);
                my_add_timer(clock_args->base, &c->fd_event, _download_page_, tv, (void*)c);
                continue;
            }

            LOG(INFO)<<"DOWNLOAD start "<<rqst->url<<" "<<rqst->ip.c_str()<<":"<<rqst->port<<" fd="<<c->sock_fd_;
            struct timeval tv = my_timeval(FLAGS_DOWN_write_timeout_ms/1000, (FLAGS_DOWN_write_timeout_ms%1000)*1000);
            my_add_event_timeout(clock_args->base, &c->fd_event, c->sock_fd_, EV_WRITE|EV_PERSIST|EV_TIMEOUT, _download_page_, tv, (void*)c);
        }

        evtimer_add(&clock_args->timer_ev, &clock_args->tv);
    }

    // 下载网页
    static void _download_page_(int sock, short events, void* arg) {
        conn* c = (conn*)arg;

        struct event_base* base = (struct event_base*)c->user_data3;
        
        if(events == EV_TIMEOUT) {              // 超时
            c->conn_stat_ = CONN_STAT_TIMEOUT;
            c->err_code_ = ETIMEDOUT;
            goto handle_result;
        }

        assert(events & (EV_READ|EV_WRITE));
    
        if(c->conn_stat_ == CONN_STAT_INIT || c->conn_stat_ == CONN_STAT_CONNECTING) {
            c->conn_stat_ = CONN_STAT_WRITING;
            c->conn_time = time(NULL);
        }
    
        if (c->conn_stat_ == CONN_STAT_WRITING) {
            if (c->send_buf_left_ > 0) {
                // 发送数据
                int n = ::write(c->sock_fd_, c->send_buf_rptr_, c->send_buf_left_);
                if(n < 0) {
                    // error
                    LOG(ERROR)<<"DOWNLOAD "<<c->sock_fd_<<" write error, errno="<<errno<<" "<<strerror(errno);
                    c->conn_stat_ = CONN_STAT_ERROR;
                    c->err_code_ = errno ;
                    goto handle_result;
                    return ;
                }
                c->send_buf_rptr_ += n;
                c->send_buf_left_ -= n;
            }
            if (c->send_buf_left_ <= 0) {
                assert(c->send_buf_left_ == 0);
                // send finished
                c->conn_stat_ = CONN_STAT_READING;
                my_event_del(&c->fd_event);
                struct timeval tv = my_timeval(FLAGS_DOWN_read_timeout_ms/1000, (FLAGS_DOWN_read_timeout_ms%1000)*1000);
                my_add_event_timeout(base, &c->fd_event, c->sock_fd_, EV_READ|EV_PERSIST|EV_TIMEOUT, _download_page_, tv, (void*)c);
                c->write_end_time = time(NULL);
            } 
            return ;
        }
    
        if (c->conn_stat_ == CONN_STAT_READING) {
            if(c->recv_begin_time == 0) c->recv_begin_time = time(NULL);
            int n = ::read(c->sock_fd_, c->recv_buf_wptr_, c->recv_buf_left_) ;
            if (n > 0) {
                c->err_code_ = 0 ;
                c->recv_buf_wptr_ += n;
                c->recv_buf_left_ -= n;
                c->recv_buf_len_  += n;
                *(c->recv_buf_wptr_) = '\0';
                if(FLAGS_DOWN_http_page_maxsize > 0 && c->recv_buf_len_ > FLAGS_DOWN_http_page_maxsize) {
                    LOG(INFO)<<"DOWNLOAD read data extend max "<<c->sock_fd_<<" "<<FLAGS_DOWN_http_page_maxsize;
                    // too long
                    c->conn_stat_ = CONN_STAT_FINISH ;
                    c->err_code_ = EMSGSIZE; // 40  EMSGSIZE Message too long
                    goto handle_result ;
                }
            }
            else if (n == 0) {
                // close        
                c->err_code_ = 0 ;
                c->conn_stat_ = CONN_STAT_FINISH ;
                *(c->recv_buf_wptr_) = '\0';
                goto handle_result ;
                
            } else if(n < 0) {
                // error
                c->conn_stat_ = CONN_STAT_ERROR;
                c->err_code_ = errno;
                LOG(INFO)<<"DOWNLOAD read error fd="<<c->sock_fd_<<" errno="<<c->err_code_;
                goto handle_result;
            }
            return ;
        }
        assert(false && "never here!!!\n") ;
    
    handle_result:
        // 删除fd事件
        event_del(&c->fd_event);

        c->recv_end_time = time(NULL);  // 结束处理时间
        assert(c->conn_stat_ == CONN_STAT_FINISH || c->conn_stat_ == CONN_STAT_ERROR || c->conn_stat_ == CONN_STAT_TIMEOUT);
        if (c->cb_result_handler_) {
            c->cb_result_handler_(c) ;
        } else {
            LOG(INFO)<<"DOWNLOAD not set result_handler, data\n"<<c->recv_buf_<<"\nlen:"<<c->recv_buf_len_;
        }
        conn_close(c) ;
        conn_free(c);
        return ;
    }

    static void _result_handler_(connection* c) {
        // 根据conn，构造一个 http_result_t
        http_request_t* rqst = (http_request_t*)c->user_data;
        http_result_t* result = new http_result_t;
        assert(result);
        result->rqst = rqst;
        result->res = rqst->res;
        result->host = rqst->host;
        result->url = rqst->url;
        result->submit_time = rqst->submit_time;
        result->write_end_time = c->write_end_time;
        result->recv_begin_time = c->recv_begin_time;
        result->recv_end_time = c->recv_end_time;
        result->error_code = c->err_code_;
        char ts[1024]={0};
        time_t now = time(NULL);
        strftime(ts, sizeof(ts),"%a %Y-%m-%d %H:%M:%S",localtime(&now));
        result->scrawltime = ts;
        if(c->conn_stat_ == CONN_STAT_FINISH) {
            result->state = http_result_t::HTTP_PAGE_OK;
            result->http_page_data = std::string(c->recv_buf_, c->recv_buf_len_);
            result->http_page_data_len = c->recv_buf_len_;
        } else {
            result->state = http_result_t::HTTP_PAGE_ERROR;
            if(c->recv_buf_len_ > 0) {
                result->http_page_data = std::string(c->recv_buf_, c->recv_buf_len_);
                result->http_page_data_len = c->recv_buf_len_;
            } else {
                result->http_page_data = "";
                result->http_page_data_len = 0;
            }
        }

        result->fetch_ip = rqst->ip;

        LOG(INFO)<<"DOWNLOAD download finished "<<result->url<<" "<<result->http_page_data_len<<" "<<(result->recv_end_time-result->submit_time);
        Downloader* downloader = (Downloader*)c->user_data2 ;
        downloader->http_result_.push(result);
    }
};

Downloader::Downloader()
  : http_request_(FLAGS_DOWN_rqst_queue_size), http_result_(FLAGS_DOWN_rslt_queue_size), is_running_(false)
{
}

Downloader::~Downloader()
{
    stop();
}

void Downloader::start()
{
    assert(0 < FLAGS_DOWN_threads && FLAGS_DOWN_threads <= 500);
    is_running_ = true;
    LOG(INFO)<<"DOWNLOAD start "<<FLAGS_DOWN_threads<<" download threads";
    for(int i=0; i<FLAGS_DOWN_threads; i++) {
        threadpool_.create_thread(_DownloaderReactor_(), (void*)this);
    }
}

void Downloader::stop()
{
    is_running_ = false;
    threadpool_.wait_all();
    LOG(INFO)<<"DOWNLOAD stop "<<FLAGS_DOWN_threads<<" download threads";
}

int Downloader::submit(http_request_t* rqst)
{
    if(rqst == NULL)
        return -1;
    http_request_.push_wait(rqst);
    return 0;
}

http_result_t* Downloader::getResult()
{
    return http_result_.pop();
}

}
}


