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

#include <boost/shared_ptr.hpp>

#include <glog/logging.h>
#include <gflags/gflags.h>

#include "md5.h"
#include "utils.h"
#include "thrift_server.h"
#include "ThriftClientWrapper.h"

using namespace std;
using namespace boost;

void load_query(std::string file, boost::shared_ptr<ThriftClientWrapper>& clients) {
    int fd = open(file.c_str(), O_RDONLY);
    if(fd == -1) {
        LOG(ERROR)<<"DISPATCHER open "<<file<<" failed, "<<errno<<" "<<strerror(errno);
        return ;
    }

    boost::shared_ptr<ThriftClientInstance> client = clients->client();
    if(!client) {
        LOG(ERROR)<<"DISPATCHER no client exist";
        return ;
    }
    char buf[1024*1024];
    ssize_t n = 0;
    ssize_t left = 0;
    int send_num = 0;
    const int switch_client_num = 1000;
    while((n=read(fd, buf+left, sizeof(buf)-left)) > 0)
    {
        char* p = buf;
        char* begin = buf;
        left += n;
        while(p < buf+left){
            if(*p == '\r' || *p == '\n'){
                std::string line = std::string(begin, p-begin);
                std::string query = line;
                if(strncmp(query.c_str(), "http://", strlen("http://")) != 0)
                {
                    unsigned char md5_val[33];
                    MD5_calc(query.c_str(), query.size(), md5_val, 32);
    
                    const static std::string default_host = "http://m.baidu.com/s?word=";
                    std::string url = default_host+query;
                    std::string userdata = "0 " + std::string((char*)md5_val, 32) ;
                    std::cout<<query<<" userdata:"<<userdata<<std::endl;
    
                    LOG(INFO)<<" send_num="<<send_num;
                    int ret = client->send(url, userdata);
                    while( ret != 0){
                        client = clients->client();
                        if(!client) {
                            LOG(ERROR)<<"DISPATCHER no client exist";
                            return ;
                        }
                        send_num = 0;
                        ret = client->send(url, userdata);
                    }
                    if(send_num++ >= switch_client_num){
                        client = clients->client();
                        if(!client) {
                            LOG(ERROR)<<"DISPATCHER no client exist";
                            return ;
                        }
                        send_num = 0;
                    }
                } 
                else{
                    client->send(query);
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

DEFINE_string(CLIENT_server_addrs, "localhost:9090", "");
DEFINE_string(CLIENT_query_file, "query.txt", "");

int main(int argc, char** argv) {
    google::ParseCommandLineFlags(&argc, &argv, true);
    // Initialize Google's logging library.
    FLAGS_logtostderr = 1;  // 默认打印错误输出
    google::InitGoogleLogging(argv[0]);

    std::vector<server_t> servers;   
    int ret = load_servers(FLAGS_CLIENT_server_addrs, servers);
    assert(ret > 0 && "load_servers failed");

    boost::shared_ptr<ThriftClientWrapper> clients(new ThriftClientWrapper);
    for(int i=0; i<servers.size(); i++) {
        server_t& server = servers[i];
        ret = clients->connect(server.ip.c_str(), server.port);
        if(ret != 0) {
            LOG(INFO)<<"DISPATCHER connect to "<<server.ip<<":"<<server.port<<" failed!";
            continue;
        }
        LOG(INFO)<<"DISPATCHER connect to "<<server.ip<<":"<<server.port;
        
    }
    load_query(FLAGS_CLIENT_query_file, clients);

    LOG(INFO)<<"finish!";
}

