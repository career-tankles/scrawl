
#include <time.h>
#include <string>

//#include "ev_ops.h"
#include "conn.h"
#include "download.h"
#include "message.h"
#include "download.h"
#include "storage.h"
#include "TimeWait.h"

//const static char* http_request_str = "GET / HTTP/1.1\r\nHost: baidu.com\r\nConnection: close\r\n\r\n" ;
const static char* http_request_str = "GET /ssid=0/from=0/bd_page_type=1/uid=0/baiduid=029071882E6772D90E7153FB1E6D957E/pu=sz%40224_220%2Cta%40middle___3_537/baiduid=029071882E6772D90E7153FB1E6D957E/w=0_10_%E7%88%B1%E4%B8%8A%E4%BA%86%E5%AE%A2%E6%9C%8D%E5%95%8A/t=wap/l=0/tc?ref=www_colorful&lid=9850718756181866726&order=10&vit=osres&tj=www_normal_10_0_10&sec=37310&di=217993a93c361350&bdenc=1&nsrc=IlPT2AEptyoA_yixCFOxXnANedT62v3IEQGG_ypOQGnb95qshbWxBcVvZyDbKXSMXpSlbTLPshUHx8Kj0WEm8xNCtKxqey6ylq HTTP/1.1\r\nHost: m.baidu.com\r\nConnection: close\r\n\r\n" ;

using namespace spider::message;
using namespace spider::download;
using namespace spider::store;

void print(http_result_t* result) {
    static int n = 0;
    std::cerr<<"NUM="<<n++<<std::endl;
    std::cerr<<"url="<<result->url<<std::endl;
    std::cerr<<"submit_time="<<result->submit_time<<std::endl;
    std::cerr<<"write_end_time="<<result->write_end_time<<std::endl;
    std::cerr<<"recv_begin_time="<<result->recv_begin_time<<std::endl;
    std::cerr<<"recv_end_time="<<result->recv_end_time<<std::endl;
    std::cerr<<"scrawltime="<<result->scrawltime<<std::endl;
    std::cerr<<"state="<<result->state<<std::endl;
    std::cerr<<"fetch_ip="<<result->fetch_ip<<std::endl;
    //std::cerr<<"http_page_data="<<result->http_page_data<<std::endl;
    std::cerr<<"http_page_data="<<result->http_page_data_len<<std::endl;
    std::cerr<<std::endl;
    std::cerr<<std::endl;
}

int main(){

    TimeWait timeWaiter;
    timeWaiter.start();

    Downloader downloader;
    downloader.start();

    struct http_request_t* http_requst = new http_request_t;
    //http_requst->set_ip("220.181.112.143");   
    http_requst->ip = "115.239.210.212";   
    http_requst->port = 80;   
    http_requst->submit_time = time(NULL);   
    http_requst->url = "http://m.baidu.com/";   
    http_requst->http_request_data = std::string(http_request_str);

    for(int i=0; i<10; i++)
        timeWaiter.add(1000+i*1000, http_requst);

    int n = 0;
    http_request_t* ready_rqst = NULL;
    while(true) {
        if((ready_rqst = (http_request_t*)timeWaiter.getReady()) == NULL) {
            std::cerr<<"waiting ..."<<std::endl;
            sleep(1);
            continue;
        }
        if(ready_rqst) {
            //downloader.submit(ready_rqst);
        }
    }

    return 0;
}


