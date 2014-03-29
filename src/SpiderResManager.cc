
#include <glog/logging.h>
#include <gflags/gflags.h>

#include "SpiderResManager.h"
#include "URI.h"
#include "conf.h"
#include "conn.h"
#include "dns.h"
#include "storage.h"
#include "download.h"
#include "TimeWait.h"

DEFINE_int32(RES_url_list_queue_size, 100000, ""); 
DEFINE_int32(RES_usleep, 10, "");

namespace spider {
  namespace scheduler {

SpiderResManager::SpiderResManager()
  : is_running_(false)
{
}
SpiderResManager::~SpiderResManager() {
    is_running_ = false;
}

int SpiderResManager::start() {

    dnsmanager.reset(new dns::DNS);
    dnsmanager->start();

    timewaiter.reset(new TimeWait);
    timewaiter->start();

    downloader.reset(new Downloader);
    downloader->start();

    storage.reset(new PageStorage);
    storage->start();

    return 0;
}
int SpiderResManager::run_loop() {
    svc();
    return 0;
}
int SpiderResManager::stop() {
    dnsmanager->stop();
    timewaiter->stop();
    downloader->stop();
    storage->stop();
    is_running_ = false;
    return 0;
}
int SpiderResManager::submit(Res* res) {
    if(!res) return -1;
    try {
        std::string url = res->url;
        if(url.empty() || strncmp(url.c_str(), "http://", strlen("http://")) != 0) {
            LOG(INFO)<<"RES url is invalid "<< url;
            delete res;
            res = NULL;
            return -1;
        }
        URI uri(url);
        LOG(INFO)<<"RES submit "<<url;
        res_list_.add(res);
        return 0;
    }catch(std::exception& e){
        LOG(ERROR)<<"RES invalid url "<<res->url<<" "<<e.what();
    }
    return -1;
}

static int _fetchFailedAllRes_(boost::shared_ptr<Website>& site, boost::shared_ptr<PageStorage>& storage) {
    // TODO: 遍历site中的url列表，生成错误结果
    LOG(INFO)<<"RES Dns error all res fetch failed "<<site->host();
    return -1;
}

static int _submitHttpRequest_(boost::shared_ptr<TimeWait>& timewaiter, boost::shared_ptr<Website>& site) {

    if(!site) return -1;

    int wait_ms = 100;
    http_request_t* http_rqst = NULL;
    int ret = site->httpRequest(http_rqst, wait_ms); // 获得一个http请求
    if(ret != 0 || http_rqst == NULL) {
        return -2;
    }
    LOG(INFO)<<"RES wait to fetch "<<http_rqst->url<<" ms:"<<wait_ms;
    ret = timewaiter->add(wait_ms, (void*)http_rqst);
    if(ret != 0) {
        return -3;
    }

    return 0;
}

static std::string _site_key_(URI& uri) {
    if(uri.host().empty()) return "";
    if(uri.port() != 0 && uri.port() != 80) {
        char buf[256];
        snprintf(buf, sizeof(buf), "%s:%d", uri.host().c_str(), uri.port());
        return std::string(buf);
    }
    return uri.host();
}

int SpiderResManager::svc() {

    DnsEntity* dns = NULL;
    struct http_request_t* http_requst = NULL;
    struct http_result_t* result = NULL;
    is_running_ = true;
    while(is_running_) {

        // 处理提交的url抓取请求
        //std::string url ;
        Res* res = NULL;
        while(res_list_.pop(res) == 0) {
            URI uri;
            uri.parse(res->url);
            std::string url = res->url;
            std::string host = uri.host();
            unsigned short port = uri.port();
            res->url = uri.path();              // url替换为path
            std::map<std::string, boost::shared_ptr<Website> >::iterator iter = sites_map_.find(host);
            if(iter == sites_map_.end()) {
                boost::shared_ptr<Website> site(new Website);
                site->host(host);
                site->port(port);
                site->addRes(res);
                std::string key = _site_key_(uri);
                sites_map_.insert(std::pair<std::string, boost::shared_ptr<Website> >(key, site));
                LOG(INFO)<<"RES Add new_site "<<url<<" size:"<<site->res_list_.size();

                dnsmanager->submit(host); // 提交DNS请求

            } else if(iter != sites_map_.end()) {
                boost::shared_ptr<Website>& site = iter->second;
                site->addRes(res);
                LOG(INFO)<<"RES Add "<<url<<" size:"<<site->res_list_.size();
                
                // 如果DNS就绪，且当前没有正在抓取的任务，则提交一个http请求
                if(site->dns_.state == DnsEntity::DNS_STATE_OK && !site->is_fetching_) {
                    _submitHttpRequest_(timewaiter, site);
                } else if(site->dns_.state == DnsEntity::DNS_STATE_ERROR) {    // DNS解析错误
                    if(site->dns_.resolve_time + site->dns_.error_retry_time <= time(NULL)) {   // DNS错误重试
                        LOG(INFO)<<"RES DNS error retry "<<site->host();
                        dnsmanager->submit(host); // 提交DNS请求
                    } else {
                        // TODO: 抓取资源直接写错误输出
                        _fetchFailedAllRes_(site, storage);
                    }
                }
            }
        }

        // 处理DNS结果
        while((dns=dnsmanager->getResult()) != NULL) {

            std::string host = dns->host;
            //print(*dns);
            // TODO: dns中没有port信息, 如何获得site
            boost::shared_ptr<Website>& site = sites_map_[host];
            if(dns->state == DnsEntity::DNS_STATE_OK) {
                site->dns_ = *dns;
                site->dns_.last_used_ip = 0;
                site->dns_.error_retry_time = FLAGS_DNS_error_retry_time; 
    
                _submitHttpRequest_(timewaiter, site);
            } else {    // DNS错误
                site->dns_ = *dns;
                LOG(INFO)<<"RES dns resolve failed "<<host;
                // TODO: 重试，重试超过3次，则将所有待抓取url，输出错误结果
                _fetchFailedAllRes_(site, storage);
            }
        }

        // 处理就绪请求
        http_request_t* rqst = NULL;
        while((rqst = (http_request_t*)timewaiter->getReady()) != NULL) {
            LOG(INFO)<<"RES submit ready http_request "<<rqst->url;
            downloader->submit(rqst);
        }

        // 处理抓取结果
        while((result = downloader->getResult()) != NULL) {
            // 重新调度此host下资源
            try{
                URI uri(result->url);
                std::string site_key = _site_key_(uri);
                boost::shared_ptr<Website>& site = sites_map_[site_key];

                site->httpResult(result);   // 更新状态
                storage->submit(result);    // 保存结果

                _submitHttpRequest_(timewaiter, site);

            } catch(...){
                continue;
            }
        } 
        usleep(FLAGS_RES_usleep);
    }

    return 0;
}

}
}

