
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <time.h>
#include <vector>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <iostream>
#include <string>
#include <glog/logging.h>
#include <gflags/gflags.h>

#include <thrift/protocol/TBinaryProtocol.h>
#include <thrift/transport/TSocket.h>
#include <thrift/transport/TTransportUtils.h>
#include "SpiderWebService.h"

#include "cJSON.h"

using namespace std;
using namespace apache::thrift;
using namespace apache::thrift::protocol;
using namespace apache::thrift::transport;
using namespace boost;
using namespace spider::webservice ;

int parse_search_list(cJSON* jroot, std::string host, std::vector<std::string>* urls=NULL, SpiderWebServiceClient* client=NULL) {
 
    cJSON* jurl = cJSON_GetObjectItem(jroot, "url");
    assert(jurl);
    cJSON* jhost = cJSON_GetObjectItem(jroot, "host");
    assert(jhost);
   
    std::string userdata;
    cJSON* juserdata = cJSON_GetObjectItem(jroot, "userdata");
    if(juserdata) {
        userdata = juserdata->valuestring;
    }

    bool has_results = false;
    cJSON* jresults = cJSON_GetObjectItem(jroot, "results");
    for(int i=0; jresults && i<cJSON_GetArraySize(jresults); i++) {
        cJSON* jresult = cJSON_GetArrayItem(jresults, i);
        cJSON* jhref = cJSON_GetObjectItem(jresult, "href");
        if(jhref == NULL || jhref->valuestring == NULL) { 
            continue;
        }
        has_results = true;
        std::string href = jhref->valuestring;
        if(!href.empty() && !host.empty() && strncmp(href.c_str(), "http://", strlen("http://")) != 0) {
            if(href[0] != '/')
                href = "/" + href;
            href = "http://" + host + href;
        }
        
        std::cout<<"AA: "<<href<<std::endl;

        if(urls) {
            urls->push_back(href);
        }

        if(client) {
            HttpRequest rqst;
            rqst.__set_url(href);
            if(!userdata.empty()) {
                std::string new_userdata(userdata);
                new_userdata[0] = 'A' + i;
                rqst.__set_userdata(new_userdata);
            }
            client->submit(rqst);
        }
    }

    return has_results?0:-1;
}

int parse_search_list_json(std::string input_json_file, SpiderWebServiceClient* client=NULL) {
    int fd = open(input_json_file.c_str(), O_RDONLY);
    if(fd == -1) {
        fprintf(stderr, "Error: %s %d-%s\n", input_json_file.c_str(), errno, strerror(errno));
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
            cJSON* jroot = cJSON_ParseWithOpts(p, &return_parse_end, 0); 
            if(jroot == NULL) {
                LOG(INFO)<<"CLIENT parse "<<records<<" records";
                break; 
            }
            std::cout<<cJSON_Print(jroot)<<std::endl;
            //getchar();
            cJSON* jurl = cJSON_GetObjectItem(jroot, "url");
            assert(jurl);
            cJSON* jhost = cJSON_GetObjectItem(jroot, "host");
            assert(jhost);

            cJSON* juserdata = cJSON_GetObjectItem(jroot, "userdata");
            assert(juserdata);

            std::string host = jhost->valuestring;
            std::string url = jurl->valuestring;
            std::string userdata ;
            if(juserdata->valuestring)
                userdata = juserdata->valuestring;

            records++;

            std::vector<std::string> urls;
            int ret = parse_search_list(jroot, host, &urls, client);
            //int ret = parse_search_list(jroot, host, NULL, client);
            if(ret == 0) {
                //print(urls);
            } else {
                LOG(INFO)<<"CLIENT parse error "<<ret;
            }
            cJSON_Delete(jroot);
            assert(return_parse_end != NULL); 
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


DEFINE_string(CLIENT_input_format, "JSON", "");
DEFINE_string(CLIENT_input_file, "output.json", "");
DEFINE_string(CLIENT_server_addr, "localhost", "");
DEFINE_int32(CLIENT_server_port, 9090, "");

int main(int argc, char** argv)
{
    google::ParseCommandLineFlags(&argc, &argv, true);
    // Initialize Google's logging library.
    FLAGS_logtostderr = 1;  // 默认打印错误输出
    google::InitGoogleLogging(argv[0]);

    shared_ptr<TTransport> socket(new TSocket(FLAGS_CLIENT_server_addr.c_str(), FLAGS_CLIENT_server_port));
    shared_ptr<TTransport> transport(new TBufferedTransport(socket));
    shared_ptr<TProtocol> protocol(new TBinaryProtocol(transport));
    SpiderWebServiceClient client(protocol);

    try {
        transport->open();

        // 解析数据
        if(FLAGS_CLIENT_input_format == "JSON") {
            parse_search_list_json(FLAGS_CLIENT_input_file, &client);
        }
    
    } catch (TException &tx) {
        printf("ERROR: %s\n", tx.what());
    }

    return 0;
}

