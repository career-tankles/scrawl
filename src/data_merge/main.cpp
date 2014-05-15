
#include <string>
#include <iostream>
#include <glog/logging.h>
#include <gflags/gflags.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "md5.h"
#include "cJSON.h"
#include "io_ops.h"
#include "ptree.hpp"
#include "httpclient.h"
#include "similarity_calculation.h"

using namespace analysis;

bool isinited = false;
word2vec_s g_s;

struct compare_s{
    int index; 
    double sim;
};

// desc
bool compare_func(const compare_s& a, const compare_s& b)
{
    return a.sim >= b.sim;
}

// 替换字符串中特征字符串为指定字符串
int ReplaceStr(char *sSrc, const char *sMatchStr, const char *sReplaceStr)
{
        int  StringLen;
        char caNewString[1024];

        char *FindPos = strstr(sSrc, sMatchStr);
        if( (!FindPos) || (!sMatchStr) )
                return -1;

        while( FindPos )
        {
                memset(caNewString, 0, sizeof(caNewString));
                StringLen = FindPos - sSrc;
                strncpy(caNewString, sSrc, StringLen);
                strcat(caNewString, sReplaceStr);
                strcat(caNewString, FindPos + strlen(sMatchStr));
                strcpy(sSrc, caNewString);

                FindPos = strstr(sSrc, sMatchStr);
        }

        return 0;
}

int calc_similarity(std::string& query, std::string& title, double& sim)
{
    int ret = word2vec_calc(g_s, query, title, sim);
    fprintf(stderr, "%s %s ret=%d sim=%f\n", query.c_str(), title.c_str(), ret, sim);
    return ret;
}

int cal_searcher_sim(std::string& query, std::string& json_str, cJSON* jorig_root)
{
    cJSON* jroot = cJSON_ParseWithOpts(json_str.c_str(), 0, 0); 
    if(jroot == NULL) {
        LOG(INFO)<<"parse json failed "<<query;
        return -1;
    }

    std::vector<compare_s> v;
    cJSON* jnew_results = cJSON_CreateArray();
    cJSON* jresults = cJSON_GetObjectItem(jroot, "items");
    for(int i=0; jresults && i<cJSON_GetArraySize(jresults); i++) {
        cJSON* jresult = cJSON_GetArrayItem(jresults, i);
        cJSON* jtitle = cJSON_GetObjectItem(jresult, "title");
        if(jtitle == NULL || jtitle->valuestring == NULL)
            continue;
        if(strlen(jtitle->valuestring) <= 0)
            continue;
        cJSON* jurl = cJSON_GetObjectItem(jresult, "url");
        cJSON* jsummary = cJSON_GetObjectItem(jresult, "summary");

        char* stitle = new char[strlen(jtitle->valuestring)+1];
        memset(stitle, 0, strlen(jtitle->valuestring)+1);
        memcpy(stitle, jtitle->valuestring, strlen(jtitle->valuestring));
        ReplaceStr(stitle, "<b>", "");
        ReplaceStr(stitle, "</b>", "");
        ReplaceStr(stitle, "<em>", "");
        ReplaceStr(stitle, "</em>", "");
        std::string title(stitle);
        delete[] stitle;

        cJSON* jnew_result = cJSON_CreateObject();
        if(jtitle && jtitle->valuestring)
            cJSON_AddStringToObject(jnew_result, "title", jtitle->valuestring); 
        if(jurl && jurl->valuestring)
            cJSON_AddStringToObject(jnew_result, "url", jurl->valuestring); 
        if(jsummary && jsummary->valuestring)
            cJSON_AddStringToObject(jnew_result, "summary", jsummary->valuestring); 

        cJSON* jfrom = cJSON_GetObjectItem(jresult, "from");
        if(jfrom != NULL && jfrom->valuestring != NULL)
            cJSON_AddStringToObject(jnew_result, "from", jfrom->valuestring); 

        // 计算相似度
        double sim = 0;
        int ret = calc_similarity(query, title, sim);
        if(ret == 0) {
            LOG(INFO)<<"calc_similarity "<<query<<" vs "<<title<<"="<<sim;
            cJSON_AddNumberToObject(jnew_result, "sim", sim); 

            compare_s cs;
            cs.index = i;
            cs.sim = sim;
            v.push_back(cs);

        } else {
            LOG(INFO)<<"calc_similarity failed  "<<query<<" "<<title;
        }
        cJSON_AddItemToArray(jnew_results, jnew_result);
    }
    cJSON_Delete(jroot);

    if(v.size() > 0) {
        // sort
        char buf[10];
        std::string searcher_sort;
        std::sort(v.begin(), v.end(), compare_func);
        for(int i=0; i<v.size(); i++) {
            compare_s& t = v[i];
            snprintf(buf, sizeof(buf), "%d", t.index);
            searcher_sort += std::string(buf, strlen(buf));
            if(i != v.size()-1)
                searcher_sort += ",";
        }
        LOG(INFO)<<"searcher_sort="<<searcher_sort;

        cJSON_AddStringToObject(jorig_root, "mso_sort", searcher_sort.c_str());
        cJSON_AddItemToObject(jorig_root, "mso_results", jnew_results);

        return 0;
    } else {
        cJSON_Delete(jnew_results);
        return -1;
    }
}

int cal_searcher_sim(std::string& query, cJSON* jroot)
{
    CHttpClient client;
    //std::string url = "http://10.129.103.17:9501/mod_searcher/Search?q=%E5%AF%84%E6%9D%8E%E5%8D%81%E4%BA%8C%E7%99%BD%E4%BA%8C%E5%8D%81%E9%9F%B5";
    std::string base_url = "http://10.129.103.17:9501/mod_searcher/Search?q=";
    std::string url = base_url + query;
    std::string resp;
    int ret = client.Get(url, resp);
    //if (ret != CURLE_OK) {
    if (ret != 0) {
        LOG(ERROR)<<"cal_searcher_sim http get "<<url<<" failed";
        return -1;
    }

    const char* beg = resp.c_str();
    const char* end = resp.c_str()+resp.size();
    febird::objbox::obj* p_obj = febird::objbox::php_load(&beg, end);
    std::string json_str;
    febird::objbox::php_save_as_json(p_obj, &json_str);

    return cal_searcher_sim(query, json_str, jroot);
}

int cal_baidu_sim(std::string& query, cJSON* jroot)
{
    cJSON* jurl = cJSON_GetObjectItem(jroot, "url");
    assert(jurl);
    std::string url = jurl->valuestring;

    if(query.empty()){
        const char* p = strstr(url.c_str(), "word=");
        if(p) {
            p += strlen("word=");
        }
        
        query = std::string(p); // TODO: parse query from url
        LOG(INFO)<<"Parse query="<<query;
    }

    std::vector<compare_s> v;
 
    cJSON* jresults = cJSON_GetObjectItem(jroot, "results");
    for(int i=0; jresults && i<cJSON_GetArraySize(jresults); i++) {
        cJSON* jresult = cJSON_GetArrayItem(jresults, i);
        cJSON* jsuburl = cJSON_GetObjectItem(jresult, "url"); // 抓取失败的没有url
        if(jsuburl == NULL || jsuburl->valuestring == NULL) { 
            LOG(INFO)<<"WARN no suburl "<<url<<" "<<i;
            continue;
        }
        std::string suburl = jsuburl->valuestring;
        if(!suburl.empty() && !suburl.empty() && strncmp(suburl.c_str(), "http://", strlen("http://")) != 0) {
            continue;
        }
        cJSON* jtitle = cJSON_GetObjectItem(jresult, "title");
        if(jtitle == NULL || jtitle->valuestring == NULL || strlen(jtitle->valuestring) <= 0) { 
            LOG(INFO)<<"WARN no title "<<url<<" "<<i<<" "<<suburl;
            continue;
        }
        if(strlen(jtitle->valuestring) <= 0)
            continue;

        char* stitle = new char[strlen(jtitle->valuestring)+1];
        memset(stitle, 0, strlen(jtitle->valuestring)+1);
        memcpy(stitle, jtitle->valuestring, strlen(jtitle->valuestring));
        ReplaceStr(stitle, "<b>", "");
        ReplaceStr(stitle, "</b>", "");
        ReplaceStr(stitle, "<em>", "");
        ReplaceStr(stitle, "</em>", "");
        std::string title(stitle);
        delete[] stitle;

        // 计算相似度
        double sim = 0;
        int ret = calc_similarity(query, title, sim);
        if(ret == 0) {
            LOG(INFO)<<"calc_similarity "<<query<<" vs "<<title<<"="<<sim;
            cJSON_AddNumberToObject(jresult, "sim", sim); 

            compare_s cs;
            cs.index = i;
            cs.sim = sim;
            v.push_back(cs);
        } else {
            LOG(INFO)<<"calc_similarity failed  "<<query<<" "<<title;
        }
        // 删除href
        cJSON_DeleteItemFromObject(jresult, "href");
    }// for

    if(v.size() > 0) {
        // sort
        char buf[10];
        std::string baidu_sort;
        std::sort(v.begin(), v.end(), compare_func);  // sorted by similarity desc
        for(int i=0; i<v.size(); i++) {
            compare_s& t = v[i];
            snprintf(buf, sizeof(buf), "%d", t.index);
            baidu_sort += std::string(buf, strlen(buf));
            if(i != v.size()-1)
                baidu_sort += ",";
        }

        unsigned char md5_val[33];
        memset(md5_val, 0, sizeof(md5_val));
        MD5_calc(query.c_str(), query.size(), md5_val, 33);
        cJSON_AddStringToObject(jroot, "md5", (const char*)md5_val);
        cJSON_AddStringToObject(jroot, "query", query.c_str());
        cJSON_AddStringToObject(jroot, "baidu_sort", baidu_sort.c_str());

        LOG(INFO)<<"baidu_sort="<<baidu_sort;

        return 0;
    } else {
        LOG(INFO)<<"WARN baidu no similarity "<<query;
        return -1;
    }
}

int parse_baidu_search_list_json(std::string input_json_file, std::string output_file) {
    int fd = open(input_json_file.c_str(), O_RDONLY);
    if(fd == -1) {
        fprintf(stderr, "Error: %s %d-%s\n", input_json_file.c_str(), errno, strerror(errno));
        return -1;
    }
    int outfd = open(output_file.c_str(), O_WRONLY|O_CREAT, S_IRWXU|S_IRWXG|S_IRWXO);
    if(outfd == -1) {
        fprintf(stderr, "Error: %s %d-%s\n", output_file.c_str(), errno, strerror(errno));
        return -2;
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
            std::string query("");
            try {
                std::string out_json_str;
                int ret = cal_baidu_sim(query, jroot);
                if(ret == 0) {
                    // 计算searcher的search结果的相似度
                    ret = cal_searcher_sim(query, jroot);
                    char* out = cJSON_PrintUnformatted(jroot);
                    std::string out_json_str = out;
                    free(out);
                    LOG(INFO)<<"json_result:"<<out_json_str;
                    io_append(outfd, out_json_str);
                    io_append(outfd, "\n", 1);
                }
            }catch(...)
            {
                LOG(ERROR)<<"Exception: "<<query;
            }

            records ++;
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
    close(outfd);
}

int main(int argc, char** argv)
{
    if(argc != 5) {
        fprintf(stderr, "Usage: %s <dict_dir> <model.bin> <search_list_file> <out_file>\n", argv[0]);
        return 1;
    }
    const char* dict = argv[1];
    const char* model_file = argv[2];
    const char* baidu_search_list_file = argv[3];
    const char* out_file = argv[4];
    int ret = word2vec_init(g_s, model_file, dict);
    fprintf(stderr, "word2vec_init = %d\n", ret);

    ret = parse_baidu_search_list_json(baidu_search_list_file, out_file); 
    LOG(INFO)<<"parse_baidu_search_list_json ret="<<ret;

    word2vec_destroy(g_s);   
 
    return 0;
}
