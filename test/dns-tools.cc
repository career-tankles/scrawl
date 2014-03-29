#include <iostream>
#include <time.h>
#include "dns.h"


using namespace spider::dns;

void print(DnsEntity& e) {

    std::cerr<<std::endl;
    std::cerr<<"DNS information:"<<std::endl;

    std::cerr<<e.host<<std::endl;
    std::cerr<<(e.state==0?"OK":"ERROR")<<std::endl;
    std::cerr<<e.resolve_time<<std::endl;
    if(!e.official_name.empty())
        std::cerr<<e.official_name<<std::endl;
    
    for (int j = 0; j < e.ips.size(); j++) {
        const std::string& ip = e.ips[j];
        std::cerr<<ip<<std::endl;
    }
    
    for (int i=0; i<e.alias.size(); i++) {
        const std::string& alias = e.alias[i];
        std::cerr<<alias<<std::endl;
    }
    std::cerr<<std::endl;
}       


int main(int argc, char** argv)
{
    DNS dnsmanager;
    dnsmanager.start();

    const int num = 1;
    for(int i=0; i<num; i++) {
        dnsmanager.submit(std::string(argv[1]));
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

