#include <sys/types.h>
#include "cJSON.h"

#include <iostream>

/*
#define cJSON_AddNullToObject(object,name)      cJSON_AddItemToObject(object, name, cJSON_CreateNull())
#define cJSON_AddTrueToObject(object,name)      cJSON_AddItemToObject(object, name, cJSON_CreateTrue())
#define cJSON_AddFalseToObject(object,name)     cJSON_AddItemToObject(object, name, cJSON_CreateFalse())
#define cJSON_AddBoolToObject(object,name,b)    cJSON_AddItemToObject(object, name, cJSON_CreateBool(b))
#define cJSON_AddNumberToObject(object,name,n)  cJSON_AddItemToObject(object, name, cJSON_CreateNumber(n))
#define cJSON_AddStringToObject(object,name,s)  cJSON_AddItemToObject(object, name, cJSON_CreateString(s))
*/

int main() {

    cJSON* obj = cJSON_CreateObject();
    cJSON_AddStringToObject(obj, "srcid", "map");
    std::cout<<cJSON_Print(obj)<<std::endl;
    //std::cout<<cJSON_PrintUnformatted(obj)<<std::endl;
    cJSON_Delete(obj);

    const int require_null_terminated = 0;
    const char* s = "{\"a\":\"aaaa\", \"b\":\"bbbb\"} {\"c\":\"ccccc\", \"d\":\"ddddd\"}";
    const char* return_parse_end= NULL;
    cJSON* obj2 = cJSON_ParseWithOpts(s, &return_parse_end, require_null_terminated);
    std::cout<<s<<std::endl;
    std::cout<<return_parse_end+1<<std::endl;
    cJSON_Delete(obj2);
    return 0;
}



