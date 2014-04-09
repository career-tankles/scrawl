/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements. See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership. The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License. You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied. See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

#include <stdio.h>
#include <unistd.h>
#include <sys/time.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <iostream>

#include <thrift/protocol/TBinaryProtocol.h>
#include <thrift/transport/TSocket.h>
#include <thrift/transport/TTransportUtils.h>

#include "SpiderWebService.h"
#include "md52.h"

using namespace std;
using namespace apache::thrift;
using namespace apache::thrift::protocol;
using namespace apache::thrift::transport;

using namespace boost;

using namespace spider::webservice ;


void splitByLine(const char* s, std::vector<std::string>& v) {
    if(!s) return ;

    std::cout<<"["<<s<<"]"<<std::endl;
    const char* start = NULL;
    const char* p = s;
    while(p && *p != '\0') {
        while(p && *p != '\0' && (*p == '\r' || *p == '\n')) p++; // 第一个非空字符
        start = p;
        while(p && *p != '\0' && *p != '\r' && *p != '\n') p++; // 起始非空字符
        if(start && p && *p != '\0' ) {
            v.push_back(std::string(start, p-start));
        } else if(start) {
            v.push_back(std::string(start));
        }   
    }   
}

void splitByTab(const char* s, std::vector<std::string>& v) {
    if(!s) return ;

    std::cout<<"["<<s<<"]"<<std::endl;
    const char* start = NULL;
    const char* p = s;
    while(p && *p != '\0') {
        while(p && *p != '\0' && (*p == '\t' || *p == '\n')) p++; // 第一个非空字符
        start = p;
        while(p && *p != '\0' && *p != '\t' && *p != '\n') p++; // 起始非空字符
        if(start && p && *p != '\0' ) {
            v.push_back(std::string(start, p-start));
        } else if(start) {
            v.push_back(std::string(start));
        }   
    }   
}

void load(std::string file, SpiderWebServiceClient& client) {
    int fd = open(file.c_str(), O_RDONLY);
    if(fd == -1) return ;
    char buf[1024*1024];
    ssize_t n = 0;
    ssize_t left = 0;
    while((n=read(fd, buf+left, sizeof(buf)-left)) > 0)
    {
        char* p = buf;
        char* begin = buf;
        left += n;
        while(p < buf+left){
            if(*p == '\r' || *p == '\n'){
                std::string line = std::string(begin, p-begin);
                std::string query = line;
                //if(strncmp(url.c_str(), "http://", strlen("http://")) != 0)
                {
                        unsigned char md5_val[33];
                        MD5_calc(query.c_str(), query.size(), md5_val, 32);
    
                        const static std::string default_host = "http://m.baidu.com/s?word=";
                        std::string url = default_host+query;
                        std::string userdata = "0 " + std::string((char*)md5_val, 32) ;
                        std::cout<<query<<" md5:"<<md5_val<<" userdata:"<<userdata<<std::endl;
    
                        HttpRequest rqst;
                        rqst.__set_url(url);
                        rqst.__set_userdata(userdata);
                        client.submit(rqst);
                } 

                while(p<buf+left) {
                    if(*p != '\r' && *p != '\n')
                        break;
                    p++;
                }
                begin = p;
                continue;
            }
            p++;
        }
        if(begin != buf && p > begin)
            memmove(buf, begin, p-begin);
    }
}



int main(int argc, char** argv) {
  shared_ptr<TTransport> socket(new TSocket("localhost", 9090));
  shared_ptr<TTransport> transport(new TBufferedTransport(socket));
  shared_ptr<TProtocol> protocol(new TBinaryProtocol(transport));
  SpiderWebServiceClient client(protocol);

  try {
    transport->open();

    load("urls.txt", client);

    transport->close();
  } catch (TException &tx) {
    printf("ERROR: %s\n", tx.what());
  }

}
