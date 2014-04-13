#ifndef __MESSAGE_USER_H_
#define __MESSAGE_USER_H_ 

#include <map>
#include <list>
#include <string>
#include <vector>
#include <boost/shared_ptr.hpp>

namespace spider {
    namespace message {

struct DnsEntity {

    enum DNS_STATE {
            DNS_STATE_INIT = -1,
            DNS_STATE_OK = 0,
            DNS_STATE_ERROR = 1,
            DNS_STATE_ERROR_RETRY = 2,
            DNS_STATE_UPDATE = 3,
    };
    DNS_STATE state;                // 状态
    std::string host;               // 主机名
    std::string official_name;      // 正式名称
    std::vector<std::string> ips;   // ip地址
    std::vector<std::string> alias; // 别名
    int resolve_time;               // 时间
    int last_used_ip;               // 当前使用ip(多个IP时轮询)
    int error_retry_time;           // 错误重试时间间隔
    int errcode;

    DnsEntity() {
        state = DNS_STATE_INIT;
        host = "";
        official_name = "";
        ips.clear();
        alias.clear();
        resolve_time = -1;
        last_used_ip = -1;
        error_retry_time = 3600;
        errcode = 0;
    }
};

struct Res {
    std::string url;
    std::string userdata;
    int err_count;
    
    Res(): url(""), userdata(""), err_count(0){}
};

class ResList 
{
public:
    ResList() {}
    ~ResList() {}

    int add(Res* res);
    int pop(Res*& res);
    size_t size() { return url_list_.size(); }

private:
    std::list<Res*> url_list_;
};

// http请求
struct http_request_t {
    std::string host;                               // host
    std::string url;                                // url
    std::string ip;                                 // IP
    unsigned short port;                            // PORT
    unsigned int submit_time;                       // 抓取数据所用时间
    std::string http_request_data;                  // HTTP请求(HTTP 协议)
    int conn_timeout;
    int recv_timeout;
    Res* res;

    http_request_t() {
        url = "";
        ip = "";
        port = 80;
        submit_time = -1;
        http_request_data = "";
        conn_timeout = 100;
        recv_timeout = 500;
    }
};

// 网页结果
struct http_result_t {

    enum HTTP_PAGE_STATE {
        HTTP_PAGE_INIT = -1,
        HTTP_PAGE_OK = 0,
        HTTP_PAGE_ERROR = 1,
    };

    std::string host;                           // host
    std::string url;                            // URL
    int submit_time;                            // 请求提交时间
    int write_end_time;                         // 写结束时间
    int recv_begin_time;                        // 开始时间
    int recv_end_time;                          // 抓取数据结束时间
    std::string scrawltime;                     // human-readable timestamp
    HTTP_PAGE_STATE state;                      // 状态
    std::string fetch_ip;                       // 抓取IP地址
    int http_page_data_len;                     // 网页数据长度
    std::string http_page_data;                 // 网页数据
    http_request_t* rqst;
    Res* res;

    int error_code;
    
    http_result_t() {
        url = "";
        submit_time = 0;
        write_end_time = 0;
        recv_begin_time = 0;
        recv_end_time = 0;
        scrawltime = "";
        state = HTTP_PAGE_INIT;
        http_page_data_len = 0;
        http_page_data = "";
        rqst = NULL;
        res = NULL;
        error_code = 0;
    }
};

struct WebsiteConfig {

};

class Website {

public:
    enum STATE {
        WEBSITE_INIT = -1,
        WEBSITE_OK = 0,
        WEBSITE_ERROR,
    };
 
    Website();

    int addRes(Res* res);   // 添加一个待抓取资源

    void addHeader(std::string& key, std::string& value);

    int dnsRequest(std::string& host);
    int dnsResult(DnsEntity* dns);

    int httpRequest(http_request_t*& rqst, int& wait_ms); // 等待wait_sec后，发起一个http请求
    int httpResult(http_result_t*& result);

    void host(std::string& host) { host_ = host; }
    void port(unsigned short port) { port_ = port; }
    void state(STATE state) { state_ = state; }

    std::string& host() { return host_; }

    size_t size() { return res_list_.size(); }

//private:
    STATE state_;
    std::string host_ ;
    unsigned short port_;
    
    bool is_fetching_;                                      // 是否正在抓取
    ResList res_list_;	  	                                // 就绪列表

    int  dns_retry_count_ ;                                 // 重试次数
    bool dns_is_resolving_;                                 // 是否正在解析DNS
    DnsEntity dns_;                                         // DNS信息

    int fetch_interval_;                                    // 抓取周期(ms)
    int last_fetch_time_;                                   // 上次抓取时间(ms)
    int error_retry_time_;                                  // 发生失败后，重试的时间间隔
    std::map<std::string, std::string> more_headers_;       // http请求需要添加的一些头部信息
    std::string userdata_;                                  // 用户数据

};

} // namespace message 

} // namespace spider

#endif

