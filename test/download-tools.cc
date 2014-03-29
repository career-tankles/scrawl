
#include <time.h>
#include <string>

//#include "ev_ops.h"
#include "conn.h"
#include "download.h"
#include "message.h"
#include "download.h"
//#include "storage.h"

//const static char* http_request_str = "GET / HTTP/1.1\r\nHost: baidu.com\r\nConnection: close\r\n\r\n" ;
const static char* http_request_str = "GET /ssid=0/from=0/bd_page_type=1/uid=0/baiduid=/pu=sz%40224_220%2Cta%40middle___3_537/baiduid=4880808961BE15D43033D05B160470A8/w=0_10_%E9%98%BF%E5%8D%A1%E5%8D%A1%E7%AC%AC%E4%B8%89%E6%96%B9/t=wap/l=1/tc?ref=www_colorful&lid=10393337082113240670&order=3&vit=osres&tj=www_normal_3_0_10&waput=2&waplogo=1&sec=37275&di=204fe824393284ae&bdenc=1&nsrc=IlPT2AEptyoA_yixCFOxXnANedT62v3IJBaOMmBH_zSv95qtva02J1ZpXTuqAp7YH5bugTCco2tJoiHqO8hmkNJOrhoqej5q7EmsxarttsPPSxQQfwJiOtmU HTTP/1.1\r\nHost: m.baidu.com\r\nConnection: close\r\n\r\n" ;

using namespace spider::message;
using namespace spider::download;

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

    Downloader downloader;
    downloader.start();

    //PageStorage storage;
    //storage.start(5);

    http_request_t* http_requst = new http_request_t;
    //http_requst->ip = "220.181.112.143";   
    http_requst->ip = "115.239.210.212";
    http_requst->port = 80;
    http_requst->submit_time = time(NULL);   
    http_requst->url = "http://m.baidu.com/";   
    http_requst->http_request_data = std::string(http_request_str);

    for(int i=0; i<1; i++)
        downloader.submit(http_requst);

    int n = 0;
    http_result_t* result = NULL;
    while(true) {
        if(++n%5 == 0) {
            //std::cerr<<"submit one request"<<std::endl;
            //downloader.submit(http_requst);
        }
        if((result = downloader.getResult()) == NULL) {
            std::cerr<<"waiting ..."<<std::endl;
            sleep(1);
            continue;
        }
        if(result) {
            print(result);
            //storage.submit(result);
        }
    }
    downloader.stop();
    //storage.stop();

    return 0;
}


