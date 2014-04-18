
#include <glog/logging.h>
#include "ThriftClientWrapper.h"

ThriftClientInstance::ThriftClientInstance() 
{

}

ThriftClientInstance::~ThriftClientInstance() 
{
}

int ThriftClientInstance::connect() 
{
    assert(ip_.c_str() && port_ > 1000);

    return connect(ip_.c_str(), port_);
}

int ThriftClientInstance::connect(const char* ip, unsigned short port) 
{
    assert(ip && port > 1000);

    time_t now = time(NULL);
    last_connect_time_ = now;
    ip_ = std::string(ip);
    port_ = port;
    try {
        boost::shared_ptr<TTransport> socket(new TSocket(ip, port));
        boost::shared_ptr<TTransport> transport(new TBufferedTransport(socket));
        boost::shared_ptr<TProtocol> protocol(new TBinaryProtocol(transport));
        boost::shared_ptr<SpiderWebServiceClient> client(new SpiderWebServiceClient(protocol));
    
        transport->open();
    
        socket_ = socket;
        transport_ = transport;
        protocol_ = protocol;
        client_ = client;
        
        status_ = TCS_OK;

        return 0;
    } catch (TException &tx) {
       LOG(ERROR)<<"ERROR: connect to "<<ip<<":"<<port<<" "<<tx.what();
    }
    status_ = TCS_ERROR;
    return -1;
}

int ThriftClientInstance::send(const std::string& url) {
    try {
        LOG(INFO)<<"DISPATCHER send to "<<ip_<<":"<<port_<<" "<<url;
        return client_->submit_url(url);
    } catch (TException &tx) {
        status_ = TCS_ERROR;
    }
    LOG(INFO)<<"DISPATCHER send to "<<ip_<<":"<<port_<<" "<<url<<" failed!\n";
    return -1;
}

int ThriftClientInstance::send(const std::string& url, const std::string& userdata) {
    try {
        HttpRequest rqst;
        rqst.__set_url(url);
        rqst.__set_userdata(userdata);
        LOG(INFO)<<"DISPATCHER send to "<<ip_<<":"<<port_<<" "<<url;
        return client_->submit(rqst);
    } catch (TException &tx) {
        status_ = TCS_ERROR;
    }
    LOG(INFO)<<"DISPATCHER send to "<<ip_<<":"<<port_<<" "<<url<<" failed!\n";
    return -1;
}

int ThriftClientInstance::close() 
{
    try {
        if(transport_)
            transport_->close();
        return 0;
    } catch (TException &tx) {
        LOG(ERROR)<<"ERROR: client close failed "<<tx.what();
    }
    return 1;
}

ThriftClientWrapper::ThriftClientWrapper() 
  : last_client_index_(0), conn_retry_interval_(60)
{
}

ThriftClientWrapper::~ThriftClientWrapper() 
{
    close();
}

int ThriftClientWrapper::connect(const char* ip, unsigned short port) 
{
  try {
    boost::shared_ptr<ThriftClientInstance> client(new ThriftClientInstance());

    int ret = client->connect(ip, port);
    clients_.push_back(client);
    return ret;
  } catch (TException &tx) {
     LOG(ERROR)<<"ERROR: connect to "<<ip<<":"<<port<<" "<<tx.what();
  }
    return -1;
}

boost::shared_ptr<ThriftClientInstance> ThriftClientWrapper::client()
{
    std::vector<boost::shared_ptr<ThriftClientInstance> >::iterator iter = clients_.begin();
    for(int i=0; i<clients_.size(); i++) {
        boost::shared_ptr<ThriftClientInstance> c = *(iter+last_client_index_);
        if(c->status() != TCS_OK) {
            LOG(ERROR)<<"ERROR: has error servers"<<clients_.size()<<" "<<conn_retry_interval_;
            if(c->connect_time() + conn_retry_interval_ >= time(NULL)) {
                LOG(ERROR)<<"reconnect "<<c->ip()<<":"<<c->port();
                c->connect();
            }
        }
        last_client_index_ ++;
        last_client_index_ %= clients_.size();
        if(c->status() == TCS_OK) {
            return c;
        }
    }

    LOG(ERROR)<<"ERROR: no alive client exist";
    usleep(100*1000);
 
    return boost::shared_ptr<ThriftClientInstance>();
}

int ThriftClientWrapper::close() 
{
    std::vector<boost::shared_ptr<ThriftClientInstance> >::iterator iter = clients_.begin();
    for(; iter!=clients_.end(); iter++) {
        boost::shared_ptr<ThriftClientInstance>& client = *iter;
        if(client)
            client->close();
    }
    return 0;
}


