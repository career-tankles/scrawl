#include <sys/types.h>
#include "cJSON.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <assert.h>
#include <iostream>
#include <stdio.h>
#include <string.h>

/*
#define cJSON_AddNullToObject(object,name)      cJSON_AddItemToObject(object, name, cJSON_CreateNull())
#define cJSON_AddTrueToObject(object,name)      cJSON_AddItemToObject(object, name, cJSON_CreateTrue())
#define cJSON_AddFalseToObject(object,name)     cJSON_AddItemToObject(object, name, cJSON_CreateFalse())
#define cJSON_AddBoolToObject(object,name,b)    cJSON_AddItemToObject(object, name, cJSON_CreateBool(b))
#define cJSON_AddNumberToObject(object,name,n)  cJSON_AddItemToObject(object, name, cJSON_CreateNumber(n))
#define cJSON_AddStringToObject(object,name,s)  cJSON_AddItemToObject(object, name, cJSON_CreateString(s))
*/

// 一次性全部读取到内容(小文件<1M)
int load(const char* file, std::string& data) {
    int fd = open(file, O_RDONLY);
    if(fd == -1) {
        fprintf(stderr, "Error: %s %d-%s\n", file, errno, strerror(errno));
        return -1;
    }

    int n = 0, len = 0;
    char buf[1024*1024];
    while((n=read(fd, buf+len, sizeof(buf)-len-1)) > 0) {
        len += n;
    }
    close(fd);

    buf[len] = '\0';
    data = std::string(buf, len);
    return 0;
}

void parse(const char* file) {
    int fd = open(file, O_RDONLY);
    if(fd == -1) {
        fprintf(stderr, "error: %s %d-%s\n", file, errno, strerror(errno));
        return ;
    }

    int n = 0, len = 0;
    char buf[1024*1024];
    while((n=read(fd, buf+len, sizeof(buf)-len-1)) > 0) {
        len += n;
        const char* p = buf;
        while(len > 0) {
            const char* return_parse_end = NULL;
            cJSON* obj3 = cJSON_ParseWithOpts(p, &return_parse_end, 0); // obj3!=null情况下，return_parse_end通常不为null
            if(obj3 == NULL)
                break;

            cJSON* jlocation = cJSON_GetObjectItem(obj3, "info");

            cJSON_Delete(obj3);
            assert(return_parse_end != NULL);   //
            len -= (return_parse_end-p);
            p = return_parse_end;

        }
        if(len > 0) {
            memmove(buf, p, len);
        }
    }
    close(fd);
}

int main() {

    cJSON* obj = cJSON_CreateObject();
    cJSON_AddStringToObject(obj, "srcid", "map");
    std::cout<<cJSON_Print(obj)<<std::endl;
    std::cout<<cJSON_PrintUnformatted(obj)<<std::endl;
    cJSON_Delete(obj);

    const int require_null_terminated = 0;
    const char* s = "{\"a\":\"aaaa\", \"b\":\"bbbb\"} {\"c\":\"ccccc\", \"d\":\"ddddd\"}";
    const char* return_parse_end = NULL;
    cJSON* obj2 = cJSON_ParseWithOpts(s, &return_parse_end, require_null_terminated);
    std::cout<<s<<std::endl;
    std::cout<<return_parse_end+1<<std::endl;
    cJSON_Delete(obj2);

    std::string json_str;
    int ret = load("/tmp/spider/data/data-00-001", json_str);
    //assert(ret == 0);

    std::cout<<"aaa="<<json_str.size()<<std::endl;

    int num = 0;
    const char* data_begin = json_str.c_str();
    const char* p = data_begin;
    while(false) {
        if(p == NULL) break;
        cJSON* obj3 = cJSON_ParseWithOpts(p, &return_parse_end, 0);
        if(obj3 == NULL)
            break;
        // ...
        cJSON_Delete(obj3);
        //std::cout<<++num<<" "<<p - data_begin<<" "<<return_parse_end-data_begin<<std::endl;
        p = return_parse_end;

    }
    std::cout<<"parse /tmp/spider/data/1"<<std::endl;
    parse("/tmp/spider/data/1");
    return 0;
}

