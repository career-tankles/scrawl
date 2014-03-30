
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <glog/logging.h>
#include <gflags/gflags.h>

#include "dns.h"
#include "conf.h"

namespace spider {
namespace dns {

class _DNSResolver_ {

public:

    void operator()(void* args) {
        DNS* dns_manager = (DNS*)args ;
        assert(dns_manager);
        while(dns_manager->is_running_) {
            std::string hostname;
            bool res = dns_manager->dns_request_.pop(hostname);
            if (!res || hostname.empty()) {
                usleep(FLAGS_DNS_nores_usleep);
                continue;
            }
            DnsEntity* dns = new DnsEntity;
            dns_resove(hostname.c_str(), dns);
            dns_manager->dns_result_.push(dns);
            usleep(FLAGS_DNS_usleep);
        }
        return;
    }

private:
    int dns_resove(const char* hostname, DnsEntity* dns) {
        assert(hostname);
        dns->host = hostname;
        dns->resolve_time= (long)time(NULL);
        struct hostent *host = gethostbyname(hostname);
        if (host != NULL) {
            dns->state = DnsEntity::DNS_STATE_OK;
            dns->official_name = host->h_name;
            
            std::string ip_str;
            for (int i = 0; host->h_addr_list[i]; i++) {
                std::string ip = inet_ntoa(*((struct in_addr *)host->h_addr_list[i]));
                ip_str += ip + ";";
                dns->ips.push_back(ip);
            }

            LOG(INFO)<<"DNS resolved "<<hostname<<" ips:"<<ip_str;
    
            for (int i = 0; host->h_aliases[i]; i++) {
                std::string alias = host->h_aliases[i];
                dns->alias.push_back(alias);
            }
            dns->last_used_ip = 0;
            dns->errcode = 0;
            return 0;
        } else {
            dns->state = DnsEntity::DNS_STATE_ERROR;
            dns->errcode = h_errno;
            LOG(INFO)<<"DNS resolved "<<hostname<<" errno:"<<dns->errcode;
            return -1;
        }
    }
}; // end of '_DNSResolver_'

DNS::DNS() 
  : dns_request_(FLAGS_DNS_rqst_queue_size), dns_result_(FLAGS_DNS_rslt_queue_size), 
    is_running_(false) {
}

DNS::~DNS() {
    stop();
}

void DNS::start() {
    assert(0 < FLAGS_DNS_threads && FLAGS_DNS_threads <= 100);
    is_running_ = true;
    LOG(INFO)<<"DNS start "<<FLAGS_DNS_threads<<" dns threads";
    for(int i=0; i<FLAGS_DNS_threads; i++) {
        threadpool_.create_thread(_DNSResolver_(), this);
    }
    return;
}

void DNS::stop() {
    is_running_ = false;
    threadpool_.wait_all();
    LOG(INFO)<<"DNS stop "<<FLAGS_DNS_threads<<" dns threads";
}

int DNS::submit(std::string hostname) {      // 提交DNS查询请求
    if(hostname.empty()) {
        return -1;
    }
    LOG(INFO)<<"DNS dns_request "<<hostname;
    dns_request_.push_wait(hostname);
    return 0;
}

DnsEntity* DNS::getResult() {       // 获得结果
    DnsEntity* dns = dns_result_.pop();
    if(dns) {
        LOG(INFO)<<"DNS dns_result "<<dns->host;
    }
    return dns;
}

}// namespace dns
}


