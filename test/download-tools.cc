
#include <time.h>
#include <string>
#include <iostream>

//#include "ev_ops.h"
#include "conn.h"
#include "download.h"
#include "message.h"
#include "download.h"

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
    std::cerr<<"http_page_data="<<result->http_page_data<<std::endl;
    std::cerr<<"http_page_data="<<result->http_page_data_len<<std::endl;
    std::cerr<<std::endl;
    std::cerr<<std::endl;
}

int main(){

    Downloader downloader;
    downloader.start();

    http_request_t* http_requst = new http_request_t;
    http_requst->ip = "115.239.210.212";
    http_requst->port = 80;
    http_requst->submit_time = time(NULL);   
    http_requst->url = "http://m.baidu.com/s?word=中文";   
    const char* http_request_str = "GET /s?word=中文 HTTP/1.1\r\n"
                                   "Host: m.baidu.com\r\n"
                                   "Connection: close\r\n"            
                                   "\r\n";
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


