
#include <stdio.h>
#include <assert.h>
#include <glog/logging.h>
#include <gflags/gflags.h>
#include "message.h"
#include "URI.h"
#include "URI2.h"
#include "conf.h"

namespace spider {
  namespace message {

int ResList::add(Res* res) {
    url_list_.push_back(res);
    return 0;
}

int ResList::pop(Res*& res) {
    if(url_list_.empty())
        return -1;

    res = url_list_.front();
    url_list_.pop_front();
    return 0;
}

Website::Website() {
    state_ = WEBSITE_INIT;
    host_ = "";
    port_ = 0;
    is_fetching_ = false;
    dns_is_resolving_ = false;
    dns_retry_count_ = 0;

    fetch_interval_ = FLAGS_DOWN_defalut_fetch_interval_sec; 
    last_fetch_time_ = time(NULL) - fetch_interval_;
    error_retry_time_ = 3600;
    userdata_ = "";
    more_headers_.clear();
}

int Website::addRes(Res* res) {
    try{
        return res_list_.add(res);
    } catch(...) {
        return -1;
    }
}
void Website::addHeader(std::string& key, std::string& value) {
    more_headers_[key] = value;
}

int Website::httpResult(http_result_t* result) {
    if(!result) return -1;
    last_fetch_time_ = result->recv_end_time; // 最后抓取时间
    is_fetching_ = false;
}

int Website::httpRequest(http_request_t*& rqst, int& wait_ms) {

    Res* res = NULL;
    int ret = res_list_.pop(res);
    if(ret != 0 || res == NULL) {
        return -1;
    }

    is_fetching_ = true;

    if(!rqst) 
        rqst = new http_request_t;
    
    rqst->res = res;

    rqst->host = host_;
    //URI uri("http", host_, res->url, port_);
    URI2 uri("http", host_, res->url, port_);
    rqst->url = uri.url();

    assert(dns_.state == DnsEntity::DNS_STATE_OK);
    assert(dns_.ips.size() > 0);
    int ip_num = dns_.ips.size();
    rqst->ip = dns_.ips[(dns_.last_used_ip++)%ip_num];  // 轮询IP
    rqst->port = port_;

    long now = time(NULL);
    rqst->submit_time = now ;  // 预计抓取时间

    // HTTP Request Headers
    int l = 0;
    char buf[10*1024];
    l += sprintf(buf+l, "GET %s HTTP/1.1\r\n", res->url.c_str());
    if(port_ == 80)
        l += sprintf(buf+l, "Host: %s\r\n", host_.c_str());
    else
        l += sprintf(buf+l, "Host: %s:%d\r\n", host_.c_str(), port_);

    if( more_headers_.count("User-Agent") <= 0) {   // 没有指定UA，则使用默认UA
        l += sprintf(buf+l, "User-Agent: %s\r\n", FLAGS_DOWN_UserAgent.c_str());
    }
    
    std::map<std::string, std::string>::iterator iter = more_headers_.begin();
    for(; iter!=more_headers_.end(); iter++) {
        l += sprintf(buf+l, "%s: %s\r\n", iter->first.c_str(), iter->second.c_str());
    }
    //l += sprintf(buf+l, "Range: bytes=0-100\r\n");
    l += sprintf(buf+l, "Connection: Close\r\n");
    l += sprintf(buf+l, "\r\n");    // END
    rqst->http_request_data = buf;

    rqst->conn_timeout = 100;
    rqst->recv_timeout = 500;  // ms

    // 抓取速度控制
    if(fetch_interval_ <= 0)
        fetch_interval_ = 10;
    wait_ms = fetch_interval_*1000/ip_num - (now - last_fetch_time_)*1000;
    if(wait_ms < 0)
        wait_ms = 0;

    return 0;
}

}
}

