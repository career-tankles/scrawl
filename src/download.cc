
#include <time.h>
#include <glog/logging.h>
#include <gflags/gflags.h>

#include "conf.h"
#include "conn.h"
#include "download.h"
#include "libevent_wrapper.h"

namespace spider {
namespace download {

class _Downloader_ 
{
    struct Args {
        void* args;
        _Downloader_* self;
    };
public:
    _Downloader_(struct event_base* base)
      : base_(base)
    {}

    void operator()(void* args) {
        _download_process_(args);
    }

    void _download_process_(void* args) {

        Downloader* downloader = (Downloader*)args;
        assert(downloader);

        while(downloader->is_running_) {
            http_request_t* rqst = downloader->http_request_.pop();
            if(rqst == NULL) {
                usleep(FLAGS_DOWN_nores_usleep);
                continue;
            }
            assert(!rqst->ip.empty() 
                    && !rqst->http_request_data.empty() 
                    && !rqst->url.empty());
            // 启动下载
            conn* c = conn_new(rqst->ip.c_str(), rqst->port);
            assert(c);
            c->user_data = (void*)rqst;                     // 记录请求
            c->user_data2 = (void*)downloader;
            c->user_data3 = (void*)this;
            c->cb_result_handler_ = _result_handler_;       // 结果处理回调函数
            c->send_buf_rptr_ = (char*)rqst->http_request_data.c_str();
            c->send_buf_left_ = rqst->http_request_data.size();
            int ret = conn_connect(c);
            if(ret != 0 && errno != EINPROGRESS) {
                // connect 出错
                LOG(INFO)<<"DOWNLOAD "<< rqst->url<<" connect to "<<rqst->ip.c_str()<<":"<<rqst->port<<" failed";
                //c->sock_fd_ = -1;
                struct timeval tv = my_timeval(0, 1000);
                my_add_timer(base_, &c->fd_event, _download_page_, tv, (void*)c);
                continue;
            }

            LOG(INFO)<<"DOWNLOAD start "<<rqst->url<<" "<<rqst->ip.c_str()<<":"<<rqst->port<<" fd="<<c->sock_fd_;
            //ret = my_add_event(base_, &c->fd_event, c->sock_fd_, EV_WRITE|EV_PERSIST|EV_TIMEOUT, _download_page_, (void*)c);
            struct timeval tv = my_timeval(FLAGS_DOWN_write_timeout_ms/1000, (FLAGS_DOWN_write_timeout_ms%1000)*1000);
            my_add_event_timeout(base_, &c->fd_event, c->sock_fd_, EV_WRITE|EV_PERSIST|EV_TIMEOUT, _download_page_, tv, (void*)c);
            usleep(FLAGS_DOWN_usleep);
        }
        LOG(INFO)<<"DOWNLOAD stop one thread: "<<pthread_self();
        return;
    }

    // TODO: 超时机制
    static void _download_page_(int sock, short events, void* arg) {
        conn* c = (conn*)arg;
        _Downloader_* self = (_Downloader_*)c->user_data3;
        
        if(events == EV_TIMEOUT) {              // 超时
            c->conn_stat_ = CONN_STAT_TIMEOUT;
            c->err_code_ = errno;
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
                //my_add_event(self->base_, &c->fd_event, c->sock_fd_, EV_READ|EV_PERSIST|EV_TIMEOUT, _download_page_, (void*)c);
                struct timeval tv = my_timeval(FLAGS_DOWN_read_timeout_ms/1000, (FLAGS_DOWN_read_timeout_ms%1000)*1000);
                my_add_event_timeout(self->base_, &c->fd_event, c->sock_fd_, EV_READ|EV_PERSIST|EV_TIMEOUT, _download_page_, tv, (void*)c);
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
                    // too long
                    c->conn_stat_ = CONN_STAT_FINISH ;
                    goto handle_result ;
                }
            }
            else if (n == 0) {
                // close        
                c->conn_stat_ = CONN_STAT_FINISH ;
                *(c->recv_buf_wptr_) = '\0';
                goto handle_result ;
                
            } else if(n < 0) {
                // error
                c->conn_stat_ = CONN_STAT_ERROR;
                c->err_code_ = errno;
                LOG(INFO)<<"DOWNLOAD read error fd="<<c->sock_fd_<<" errcode="<<c->err_code_;
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

private:
    struct event_base* base_;
};

class _DownloaderReactor_
{
public:
    void operator()(void* args)
    {
        _start_threads(args);
    }

    void* _start_threads(void* args)
    {
        Downloader* downloader = (Downloader*)args;
        assert(downloader);

        base_ = event_base_new();

        threadpool threadpool_;
        threadpool_.create_thread(_Downloader_(base_), (void*)downloader);

        // 添加timer，避免无抓取任务时event_base_dispatch循环退出
        struct timeval tv; tv.tv_sec=1; tv.tv_usec = 0 ;
        my_add_timer(base_, &clockevent_, _clock_handler, tv, this);

        event_base_dispatch(base_); 
    }

    static void _clock_handler(const int fd, const short which, void *args) 
    {
        _DownloaderReactor_* self = (_DownloaderReactor_*)args;
        struct timeval tv; tv.tv_sec=FLAGS_DOWN_clock_interval_sec; tv.tv_usec = FLAGS_DOWN_clock_interval_us ;
        my_add_timer(self->base_, &self->clockevent_, _clock_handler, tv, args);
    }

private:
    struct event_base* base_;
    struct event clockevent_;
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


