// This autogenerated skeleton file illustrates how to build a server.
// You should copy it to another filename to avoid overwriting it.


#include <thrift/concurrency/ThreadManager.h>
#include <thrift/concurrency/PosixThreadFactory.h>
#include <thrift/protocol/TBinaryProtocol.h>
#include <thrift/server/TSimpleServer.h>
#include <thrift/server/TThreadPoolServer.h>
#include <thrift/server/TThreadedServer.h>
#include <thrift/transport/TServerSocket.h>
#include <thrift/transport/TTransportUtils.h>
#include <thrift/transport/TBufferTransports.h>

#include <glog/logging.h>
#include <gflags/gflags.h>

#include "conf.h"
#include "threadpool.h"
#include "SpiderWebService.h"
#include "SpiderResManager.h"
#include "message.h"

using namespace ::apache::thrift;
using namespace ::apache::thrift::protocol;
using namespace ::apache::thrift::transport;
using namespace ::apache::thrift::server;
using namespace ::apache::thrift::concurrency;

using boost::shared_ptr;

using namespace ::spider::webservice;
using namespace ::spider::message;
using namespace ::spider::scheduler;

class SpiderWebServiceHandler : virtual public SpiderWebServiceIf {
 public:
  SpiderWebServiceHandler() {
    // Your initialization goes here
  }
  SpiderWebServiceHandler(boost::shared_ptr<SpiderResManager> spider) 
    : spider_(spider)
  {
    // Your initialization goes here
  }


  int32_t setConfig(const SiteConfig& site_cfg) {
    // Your implementation goes here
    printf("setConfig\n");
  }

  int32_t submit(const HttpRequest& rqst) {
    Res* res = new Res();
    res->url = rqst.url;
    res->userdata = rqst.userdata;
    return spider_->submit(res);
  }

  int32_t submit_url(const std::string& url) {
    Res* res = new Res();
    res->url = url;
    res->userdata = "";
    return spider_->submit(res);

  }

private:
    boost::shared_ptr<SpiderResManager> spider_;

};

struct _spider_thread_ {
    void operator()(void* args) {
        boost::shared_ptr<SpiderResManager> spider = *(boost::shared_ptr<SpiderResManager>*)args;
        spider->start();
        spider->run_loop();
        //spider->stop();
    }
};

int main(int argc, char **argv) {
    google::ParseCommandLineFlags(&argc, &argv, true);
    // Initialize Google's logging library.
    FLAGS_logtostderr = 1;
    google::InitGoogleLogging(argv[0]);

    output_config();

    boost::shared_ptr<SpiderResManager> spider(new SpiderResManager);
    threadpool threadpool;
    threadpool.create_thread(_spider_thread_(), (void*)&spider);

    // thrift service 
    int port = FLAGS_SERVER_thrift_port;
    shared_ptr<SpiderWebServiceHandler> handler(new SpiderWebServiceHandler(spider));
    shared_ptr<TProcessor> processor(new SpiderWebServiceProcessor(handler));
    shared_ptr<TServerTransport> serverTransport(new TServerSocket(port));
    shared_ptr<TTransportFactory> transportFactory(new TBufferedTransportFactory());
    shared_ptr<TProtocolFactory> protocolFactory(new TBinaryProtocolFactory());
 
    //TSimpleServer server(processor, serverTransport, transportFactory, protocolFactory);
    int workerCount = FLAGS_SERVER_thrift_threadnum;
    shared_ptr<ThreadManager> threadManager =
      ThreadManager::newSimpleThreadManager(workerCount);
    shared_ptr<PosixThreadFactory> threadFactory =
      shared_ptr<PosixThreadFactory>(new PosixThreadFactory());
    threadManager->threadFactory(threadFactory);
    threadManager->start();
    TThreadPoolServer server(processor,
                             serverTransport,
                             transportFactory,
                           protocolFactory,
                           threadManager);
  /*
    TThreadedServer server(processor,
                         serverTransport,
                         transportFactory,
                         protocolFactory);

  */

  
    printf("Starting the server...\n");
    server.serve();
    printf("done.\n");

    return 0;
}

  
  
  
  
  
  
  
  
  
  
  
