#ifndef __DOWNLOADER_H__
#define __DOWNLOADER_H__

#include <event.h>
#include <boost/shared_ptr.hpp>

#include "simple_queue.h"
#include "threadpool.h"
//#include "ev_ops.h"
#include "message.h"

using namespace ::spider::message;

namespace spider {
namespace download {

class Downloader
{
public:
    Downloader();
    ~Downloader();

    void start();
    void stop();

    int submit(http_request_t* rqst);
    http_result_t* getResult();

private:
    ptr_queue<http_request_t> http_request_;
    ptr_queue<http_result_t> http_result_;
    threadpool threadpool_;
    bool is_running_;

    friend class _DownloaderReactor_;
    friend class _Downloader_;

};

}

}

#endif

