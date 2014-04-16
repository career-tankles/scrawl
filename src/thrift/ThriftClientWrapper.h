#ifndef _THRIFT_CLIENT_WRAPPER_
#define _THRIFT_CLIENT_WRAPPER_

#include <thrift/protocol/TBinaryProtocol.h>
#include <thrift/transport/TSocket.h>
#include <thrift/transport/TTransportUtils.h>

#include <list>
#include <vector>
#include <string>
#include <boost/shared_ptr.hpp>

#include "SpiderWebService.h"

using namespace apache::thrift;
using namespace apache::thrift::protocol;
using namespace apache::thrift::transport;

using namespace std;
using namespace boost;
using namespace spider::webservice ;

enum TCS_STATUS{
    TCS_OK = 0,
    TCS_ERROR,
};

class ThriftClientInstance
{
public:
    ThriftClientInstance();
    ~ThriftClientInstance();

    int connect(const char* ip, unsigned short port=9090);
    int connect();
    //boost::shared_ptr<SpiderWebServiceClient> client();
    int close() ;
    int send(std::string& url);
    int send(std::string& url, std::string& userdata);

    std::string ip() { return ip_; }
    unsigned short port() { return port_; }

    TCS_STATUS status() { return status_; }
    time_t connect_time() { return last_connect_time_; }

private:
    boost::shared_ptr<TTransport> socket_;
    boost::shared_ptr<TTransport> transport_;
    boost::shared_ptr<TProtocol> protocol_;
    boost::shared_ptr<SpiderWebServiceClient> client_;

    std::string ip_;
    unsigned short port_;

    TCS_STATUS status_;
    time_t last_connect_time_;
};

class ThriftClientWrapper
{
public:
    ThriftClientWrapper();
    ~ThriftClientWrapper();

    int connect(const char* ip="localhost", unsigned short port=9090);
    boost::shared_ptr<ThriftClientInstance> client();
    int close() ;

private:
    std::vector< boost::shared_ptr<ThriftClientInstance> > clients_;
    size_t last_client_index_ ;
    unsigned int conn_retry_interval_;

};



#endif

