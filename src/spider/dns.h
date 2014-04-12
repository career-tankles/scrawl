#ifndef __DNS_H__
#define __DNS_H__

#include <string>

#include "simple_queue.h"
#include "threadpool.h"
#include "message.h"

using namespace ::spider::message;

namespace spider {
namespace dns {

    class DNS 
    {
    public:
        DNS();
        ~DNS();
    
        void start();
        void stop();
    
        int submit(std::string host);          // 提交DNS查询请求
        DnsEntity* getResult();                 // 获得结果

    //protected:
        bool is_running_;
        simple_queue<std::string> dns_request_;
        ptr_queue<DnsEntity> dns_result_;
        threadpool threadpool_;

        friend class __DNSResolver_;
    };
} // end of dns

}

#endif

