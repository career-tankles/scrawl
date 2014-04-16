#ifndef _THRIFT_SERVER_H_
#define _THRIFT_SERVER_H_

#include <string>
#include <vector>

struct server_t
{
    std::string ip;
    unsigned short port;
};

// str like: host1:9090,host2:9090
static int load_servers(std::string& addr_list, std::vector<server_t>& servers)
{
    std::vector<std::string> v;
    splitByChar(addr_list.c_str(), v, ',');
    for(int i=0; i<v.size(); i++) {
        std::string str = v[i];
        std::vector<std::string> v2;
        splitByChar(str.c_str(), v2, ':');
        assert(v2.size() == 2);

        server_t s;
        s.ip = v2[0];
        s.port = atoi(v2[1].c_str());
        assert(!s.ip.empty() && s.port>1000);
        servers.push_back(s);
    }
    return servers.size();
}

#endif

