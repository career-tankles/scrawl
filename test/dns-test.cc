#include "dns.h"
#include <iostream>
#include <time.h>


using namespace dns;

void print(DnsEntity& dns) {

    std::cerr<<std::endl;
    std::cerr<<"DNS information:"<<std::endl;

    std::cerr<<dns.host<<std::endl;
    std::cerr<<(dns.state==0?"OK":"ERROR")<<std::endl;
    std::cerr<<dns.resolve_time<<std::endl;
    if(!dns.official_name.empty())
        std::cerr<<dns.official_name<<std::endl;
    
    for (int j = 0; j < dns.ips.size(); j++) {
        const std::string& ip = dns.ips[j];
        std::cerr<<ip<<std::endl;
    }
    
    for (int i=0; i<dns.alias.size(); i++) {
        const std::string& alias = dns.alias[i];
        std::cerr<<alias<<std::endl;
    }
    std::cerr<<std::endl;
}       


int main()
{
    dns::DNS dnsmanager;
    dnsmanager.start(1);

    const int num = 10;
    std::string host = "www.baidu.com";
    for(int i=0; i<num; i++) {
        dnsmanager.submit(host);
    }

    int n = 0;
    while(n < num) {
        DnsEntity* dns = dnsmanager.getResult();
        if(dns) {
            n++;
            print(*dns);  
        } else {
            //std::cerr<<"result null"<<std::endl;
            usleep(100);
        }
    }
    std::cerr<<"stoping"<<std::endl;
    dnsmanager.stop();
    return 0;
}

