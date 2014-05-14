
#include <glog/logging.h>
#include <gflags/gflags.h>

#include "SpiderResManager.h"
#include "URI.h"
#include "URI2.h"
#include "conf.h"
#include "conn.h"
#include "dns.h"
#include "storage.h"
#include "download.h"
#include "TimeWait.h"
#include "io_ops.h"
#include "cJSON.h"

namespace spider {
  namespace scheduler {

SpiderResManager::SpiderResManager()
  : website_cfg_(10000), is_running_(false)
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
    is_running_ = false;
    LOG(INFO)<<"SpiderResManager::stop ";
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
        //URI uri(url);
        URI2 uri(url);
        LOG(INFO)<<"RES submit "<<url<<" userdata:"<<res->userdata;
        res_list_.add(res);
        return 0;
    }catch(std::exception& e){
        LOG(ERROR)<<"RES invalid url "<<res->url<<" "<<e.what();
    }
    return -1;
}

static int _submitDnsRequest_(boost::shared_ptr<DNS>& dnsmanager, boost::shared_ptr<Website>& site) {
    time_t now = time(NULL);

    if(!site || !dnsmanager) 
        goto NO_SUBMIT_DNS;

    if (site->dns_is_resolving_) 
        goto NO_SUBMIT_DNS;

    if(site->dns_.state == DnsEntity::DNS_STATE_INIT)
        goto SUBMIT_DNS;

    // DNS错误重试
    if(site->dns_.state == DnsEntity::DNS_STATE_ERROR && site->dns_.resolve_time + FLAGS_DNS_error_retry_time_sec <= now)
    {
        site->dns_.state = DnsEntity::DNS_STATE_ERROR_RETRY;
        goto SUBMIT_DNS;
    }

    // DNS更新
    if(site->dns_.state == DnsEntity::DNS_STATE_OK && site->dns_.resolve_time + FLAGS_DNS_update_time_sec <= now) {
        LOG(INFO)<<"DNS update "<<site->host()<<" "<<site->dns_.resolve_time<<" "<<FLAGS_DNS_update_time_sec<<" "<<now;
        site->dns_.state = DnsEntity::DNS_STATE_UPDATE;
        goto SUBMIT_DNS;
    }

NO_SUBMIT_DNS:
    return -1;

SUBMIT_DNS:
    std::string host = site->host();
    if(!host.empty()) {
        site->dns_is_resolving_ = true;
        dnsmanager->submit(host);          // 提交DNS请求
        return 0;
    }
    return -2;
}

static int _dnsResult_(boost::shared_ptr<DNS>& dnsmanager, boost::shared_ptr<Website>& site, DnsEntity* dns) {
    if(!site || !dns) return -1;

    assert(site->dns_is_resolving_); 

    site->dns_ = *dns;
    site->dns_.last_used_ip = 0;
    site->dns_is_resolving_ = false;

    delete dns; dns=NULL;

    // 重试次数
    if(site->dns_.state == DnsEntity::DNS_STATE_ERROR) {
        if(site->dns_retry_count_ < FLAGS_DNS_retry_max) {
            site->dns_retry_count_ ++;
            site->dns_.state = DnsEntity::DNS_STATE_ERROR_RETRY;
            std::string host = site->host();
            if(!host.empty()) {
                site->dns_is_resolving_ = true;
                dnsmanager->submit(host);          // 提交DNS请求
                return 0;
            }
        }
    }

    if(site->dns_.state == DnsEntity::DNS_STATE_OK) {
        site->dns_retry_count_ = 0;
    }
}

static int _resToHttpResult_(Res* res, http_result_t*& result) {
    assert(res);

    if(result == NULL) 
        result = new http_result_t;

    assert(result);
    time_t now = time(NULL);

    result->res = res;
    result->rqst = NULL;
    result->url = res->url;
    result->state = http_result_t::HTTP_PAGE_ERROR;
    result->submit_time = now;
    result->write_end_time = now;
    result->recv_begin_time = now;
    result->recv_end_time = now;
    result->scrawltime = "todo";

    return 0;
}

static int _fetchFailedAllRes_(boost::shared_ptr<Website>& site, boost::shared_ptr<PageStorage>& storage) {
    if(!site || !storage) return -1;

    // 遍历site中的url列表，生成错误结果
    LOG(INFO)<<"RES Dns error all res fetch failed "<<site->host();

    Res* res = NULL;
    while(site->res_list_.pop(res) == 0) {
        assert(res);

        http_result_t* result = NULL;
        _resToHttpResult_(res, result);       

        site->httpResult(result);   // 更新状态
        storage->submit(result);
    }
    
    return -1;
}

static int _submitHttpRequest_(boost::shared_ptr<TimeWait>& timewaiter, boost::shared_ptr<Website>& site, boost::shared_ptr<PageStorage>& storage) {

    if(!site) return -1;

    if(site->dns_.state == DnsEntity::DNS_STATE_ERROR) {    // DNS解析错误
        _fetchFailedAllRes_(site, storage);
    }
    if(site->dns_.state == DnsEntity::DNS_STATE_OK && !site->is_fetching_) { // DNS就绪, 且当前未抓取
        int wait_ms = 100;
        http_request_t* http_rqst = NULL;
        int ret = site->httpRequest(http_rqst, wait_ms); // 获得一个http请求
        if(ret != 0 || http_rqst == NULL) {
            return -2;
        }
        LOG(INFO)<<"RES wait to fetch "<<http_rqst->url<<" ms:"<<wait_ms<<" res_num="<<site->size();
        ret = timewaiter->add(wait_ms, (void*)http_rqst);
        return ret;
    }
    return -100;
}

//static std::string _site_key_(URI& uri) {
static std::string _site_key_(URI2& uri) {
    if(uri.host().empty()) return "";
    if(uri.port() != 0 && uri.port() != 80) {
        char buf[256];
        snprintf(buf, sizeof(buf), "%s:%d", uri.host().c_str(), uri.port());
        return std::string(buf);
    }
    return uri.host();
}

int _res_to_str_(Res* res, std::string& s) {
    if(!res) return -1;
    if(res->url.empty()) return -2;

    cJSON* jnew_root = cJSON_CreateObject();
    cJSON_AddStringToObject(jnew_root, "url", res->url.c_str());
    if(!res->userdata.empty())
        cJSON_AddStringToObject(jnew_root, "userdata", res->userdata.c_str());
    if(res->err_count != 0)
        cJSON_AddNumberToObject(jnew_root, "err_count", res->err_count);

    char* out = cJSON_PrintUnformatted(jnew_root);
    s = out;
    free(out);
    cJSON_Delete(jnew_root);

    return s.size();
}

int _load_dumps_(std::string dump_file, SpiderResManager& spider) {
    int fd = open(dump_file.c_str(), O_RDONLY);
    if(fd == -1) {
        fprintf(stderr, "Error: %s %d-%s\n", dump_file.c_str(), errno, strerror(errno));
        return -1;
    }

    int records = 0;
    int n = 0, len = 0;
    char buf[8*1024*1024];
    while((n=read(fd, buf+len, sizeof(buf)-len-1)) > 0) {
        len += n;
        const char* p = buf;
        while(len > 0) {
            const char* return_parse_end = NULL;
            cJSON* obj = cJSON_ParseWithOpts(p, &return_parse_end, 0); 
            if(obj == NULL) {
                LOG(INFO)<<"EXTRACTOR parse "<<records<<" records";
                break; 
            }
            
            cJSON* jurl = cJSON_GetObjectItem(obj, "url");
            if(jurl == NULL) {
                break;
            }
            cJSON* juserdata = cJSON_GetObjectItem(obj, "userdata");
            cJSON* jerr_count = cJSON_GetObjectItem(obj, "err_count");

            Res* res = new Res;
            assert(res);
            res->url = jurl->valuestring;
            if(juserdata) {
                res->userdata = juserdata->valuestring;
            }
            if(jerr_count) {
                res->err_count = jerr_count->valueint;
            }

            LOG(INFO)<<"RES load dump "<<records<<" "<<res->url;
            spider.submit(res);
            records++;

            cJSON_Delete(obj);
            assert(return_parse_end != NULL);   //
            len -= (return_parse_end-p);
            p = return_parse_end;

            //getchar();
        }   
        if(len > 0) {
            memmove(buf, p, len);
        }   
    }
    close(fd);
}

int SpiderResManager::svc() {

    Res* res = NULL;
    DnsEntity* dns = NULL;
    struct http_request_t* http_requst = NULL;
    struct http_result_t* result = NULL;

    // TODO: 加载dump file
    std::string dump_file = FLAGS_RES_dump_file;

    struct stat sb;
    if(stat(dump_file.c_str(), &sb) != -1) {
        assert((sb.st_mode&S_IFMT) == S_IFREG); // 普通文件
        if(sb.st_size > 0) {
            int ret = _load_dumps_(dump_file, *this);
            if(ret == 0) unlink(dump_file.c_str());
        }
    }
    
    LOG(INFO)<<"Spider start ...";

    is_running_ = true;
    while(is_running_) {

        // 处理提交的url抓取请求
        int max_slot = FLAGS_RES_add_res_each_loop;   // 每次最多处理res数量
        assert(max_slot > 0);
        while(max_slot-- > 0 && res_list_.pop(res) == 0) {
            //URI uri;
            URI2 uri;
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

                _submitDnsRequest_(dnsmanager, site);   // 交DNS请求

                std::string key = _site_key_(uri);
                sites_map_.insert(std::pair<std::string, boost::shared_ptr<Website> >(key, site));
                LOG(INFO)<<"RES Add new_site "<<url<<" size:"<<site->res_list_.size();

            } else if(iter != sites_map_.end()) {
                boost::shared_ptr<Website>& site = iter->second;
                site->addRes(res);
                LOG(INFO)<<"RES Add "<<url<<" size:"<<site->res_list_.size();
                
                _submitDnsRequest_(dnsmanager, site);
                _submitHttpRequest_(timewaiter, site, storage);
            }
        }

        // 处理DNS结果
        while((dns=dnsmanager->getResult()) != NULL) {

            std::string host = dns->host;
            boost::shared_ptr<Website>& site = sites_map_[host];  // TODO: dns中没有port信息, 如何获得site
            _dnsResult_(dnsmanager, site, dns);
            _submitHttpRequest_(timewaiter, site, storage);
            
        }

        // 处理就绪请求
        http_request_t* rqst = NULL;
        while((rqst = (http_request_t*)timewaiter->getReady()) != NULL) {
            LOG(INFO)<<"RES http_request ready "<<rqst->url;
            downloader->submit(rqst);
        }

        // 处理抓取结果
        while((result = downloader->getResult()) != NULL) {
            try{
                //URI uri(result->url);
                URI2 uri(result->url);
                std::string site_key = _site_key_(uri);
                boost::shared_ptr<Website>& site = sites_map_[site_key];

                site->httpResult(result);   // 更新状态
                storage->submit(result);    // 保存结果

                // 重新调度此host下资源
                _submitHttpRequest_(timewaiter, site, storage);

            } catch(...){
                continue;
            }
        } 
        usleep(FLAGS_RES_usleep);

    } // while(is_running_)

    // will be exiting

    // TODO: to store data
    //
    LOG(INFO)<<"RES will be exiting, dump res";
 
    // 处理抓取结果
    while((result = downloader->getResult()) != NULL) {
        storage->submit(result);    // 保存结果
    } 
  
    int fd = open(dump_file.c_str(), O_WRONLY|O_CREAT, S_IRWXU|S_IRWXG|S_IRWXO);
    if(fd == -1) {
        LOG(ERROR)<<"RES open file failed "<<dump_file<<" "<<errno<<" "<<strerror(errno);
        return -1;
    }

    int records = 0;
    std::string s;
    // 处理就绪请求
    http_request_t* rqst = NULL;
    while((rqst = (http_request_t*)timewaiter->getReady()) != NULL) {
        std::string host = rqst->host;
        Res* res = rqst->res;
        res->url = "http://" + host + res->url;
        s.clear();
        int n = _res_to_str_(res, s);
        if(n > 0) {
            records ++;
            io_append(fd, s);
            io_append(fd, "\n", strlen("\n"));
        }
        delete rqst; rqst = NULL;
    }

    while(res_list_.pop(res) == 0) {
        s.clear();
        int n = _res_to_str_(res, s);
        if(n > 0) {
            records ++;
            io_append(fd, s);
            io_append(fd, "\n", strlen("\n"));
        }
    }
    std::map<std::string, boost::shared_ptr<Website> >::iterator iter = sites_map_.begin();
    for(; iter!=sites_map_.end(); iter++) {
        boost::shared_ptr<Website>& site = iter->second;
        while(site->popRes(res) == 0) {
            res->url = "http://" + site->host() + res->url;
            s.clear();
            int n = _res_to_str_(res, s);
            if(n > 0) {
                records ++;
                io_append(fd, s);
                io_append(fd, "\n", strlen("\n"));
            }
        }
    }
    close(fd);

    LOG(INFO)<<"RES dump "<<records<<" to "<<dump_file;

    dnsmanager->stop();
    timewaiter->stop();
    downloader->stop();
    storage->stop();

    LOG(INFO)<<"Spider stoped!";

    return 0;
}

}
}

