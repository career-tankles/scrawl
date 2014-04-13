#ifndef _CFG_TPL_H_
#define _CFG_TPL_H_


#include <map>
#include <vector>
#include <string>
#include <iostream>

#include <assert.h>

#include <glog/logging.h>
#include <gflags/gflags.h>

#include "cJSON.h"
#include "io_ops.h"

struct cfg_tpl_item {
    std::string _key_ ;
    cJSON* jnode;
    cfg_tpl_item(): _key_(""), jnode(NULL) {}
};

struct cfg_tpl_chain {
    std::string _name_;                 // 识别命中的是哪个
    std::string _type_;                 // HEADER or BODY
    std::string _nodeset_xpath_;        // type=BODY时有用
    std::vector<std::string> _keys_;    // type=BODY时有用, type=HEADER时记录要解析的header-name
    cJSON* jnode;                       // 对应的cJson节点

    // type=BODY时，记录每个key对应的解析器
    std::map<std::string, struct cfg_tpl_item> results;

    cfg_tpl_chain(): _name_(""), _type_(""), _nodeset_xpath_(""), jnode(NULL) {}
};

struct cfg_tpl_host {
    std::string host;
    cJSON* jroot;

    std::vector<struct cfg_tpl_chain> chains;

    cfg_tpl_host(): host(""), jroot(NULL) {}

    void release() {
        if(jroot) {
            cJSON_Delete(jroot);
            jroot = NULL;
        }
    }
};

static int parse_cfg(const char* json_str, std::string& host, struct cfg_tpl_host& cfg) {
    if(!json_str) {
        LOG(ERROR)<<"TPL invalid json_str(null)";
        return -1;
    }
    cJSON* jroot = cJSON_Parse(json_str);
    if(jroot == NULL) {
        LOG(ERROR)<<"TPL Error: json in invalid "<<((cJSON_GetErrorPtr()-80)>json_str?(cJSON_GetErrorPtr()-80):json_str);
        return -2;
    }

    cJSON* jhost = cJSON_GetObjectItem(jroot, "host");
    if(jhost == NULL ) {
        LOG(ERROR)<<"TPL Error field \"host\" not exist ";
        return -3;
    }
    host = jhost->valuestring;
    cfg.host = host;
    cfg.jroot = jroot;     // 后续释放资源

    fprintf(stderr, "HOST:%s\n", cfg.host.c_str());

    cJSON* jchains = cJSON_GetObjectItem(jroot, "chains");
    if(jchains == NULL) {
        LOG(ERROR)<<"TPL Error field \"chains\" not exist ";
        return -3;
    }

    for (int i = 0 ; i < cJSON_GetArraySize(jchains) ; i++) {
        cJSON* jchain = cJSON_GetArrayItem(jchains, i);

        cfg.chains.push_back(cfg_tpl_chain());
        struct cfg_tpl_chain& tpl_chain = cfg.chains[cfg.chains.size()-1];

        cJSON* jname = cJSON_GetObjectItem(jchain, "_name_");
        if(jname == NULL ) {
            LOG(ERROR)<<"TPL Error field \"_name_\" not exist ";
            return -4;
        }

        cJSON* jtype = cJSON_GetObjectItem(jchain, "_type_");
        if(jtype == NULL ) {
            LOG(ERROR)<<"TPL Error field \"_type_\" not exist, HEADER or BODY ";
            return -5;
        }

        std::string name = jname->valuestring;
        std::string type = jtype->valuestring;
        if(type == "HEADER") {
            tpl_chain._type_ = type;
            tpl_chain._name_ = name;
            tpl_chain.jnode = jchain;
 
            cJSON* jheaders = cJSON_GetObjectItem(jchain, "_headers_");
            if(jheaders == NULL) {
                LOG(ERROR)<<"TPL Error field \"_headers_\" not exist ";
                return -7;
            }
            for (int i = 0 ; i < cJSON_GetArraySize(jheaders) ; i++) {
                cJSON* jheader = cJSON_GetArrayItem(jheaders, i);
                assert(jheader);
                assert(jheader->valuestring);
                
                LOG(INFO)<<"TPL add header "<<jheader->valuestring;
                tpl_chain._keys_.push_back(jheader->valuestring);
            }

        } else if (type == "BODY") {

            cJSON* jnodeset_xpath = cJSON_GetObjectItem(jchain, "_nodeset_xpath_");
            if(jnodeset_xpath == NULL ) {
                LOG(ERROR)<<"TPL Error field \"_nodeset_xpath_\" not exist ";
                return -6;
            }
    
            tpl_chain._type_ = type;
            tpl_chain._name_ = name;
            tpl_chain._nodeset_xpath_ = jnodeset_xpath->valuestring;
            tpl_chain.jnode = jchain;
    
            cJSON* jkeys = cJSON_GetObjectItem(jchain, "_keys_");
            if(jkeys == NULL) {
                LOG(ERROR)<<"TPL Error field \"_keys_\" not exist ";
                return -7;
            }
            for (int i = 0 ; i < cJSON_GetArraySize(jkeys) ; i++) {
                cJSON* jkey = cJSON_GetArrayItem(jkeys, i);
                assert(jkey && jkey->valuestring);
                
                LOG(ERROR)<<"TPL add key "<<jkey->valuestring;
                tpl_chain._keys_.push_back(jkey->valuestring);
            }
        
            cJSON* jresults = cJSON_GetObjectItem(jchain, "_results_");
            for (int i = 0 ; jresults && i < cJSON_GetArraySize(jresults) ; i++) {
                cJSON* jresult = cJSON_GetArrayItem(jresults, i);
        
                cJSON* j_key_= cJSON_GetObjectItem(jresult, "_key_");
                if(j_key_== NULL ) {
                    LOG(ERROR)<<"TPL Error field \"_key_\" not exist";
                    return -1;
                }
        
                struct cfg_tpl_item tpl_item;
                tpl_item._key_ = j_key_->valuestring;
                tpl_item.jnode = jresult;
                LOG(INFO)<<"TPL add parser "<<tpl_item._key_;
                tpl_chain.results.insert(std::pair<std::string, struct cfg_tpl_item>(tpl_item._key_, tpl_item));
            }
        } else {
            assert(false);
        }
    }
    return 0;

}

static void print(std::vector<std::string>& v) {
    for(int i=0; i<v.size(); i++)
        std::cout<<v[i]<<std::endl;
    std::cout<<std::endl;
}

static int load_tpls(std::string file, std::map<std::string, struct cfg_tpl_host>& maps_tpls) {
    int fd = open(file.c_str(), O_RDONLY);
    if(fd == -1) return -1;
    ssize_t n = 0;
    ssize_t left = 0;
    char buf[1024*1024];
    while((n=read(fd, buf+left, sizeof(buf)-left)) > 0)
    {
        char* p = buf;
        char* begin = buf;
        left += n;
        while(p < buf+left){
            if(*p == '\r' || *p == '\n'){
                std::string tpl_file = std::string(begin, p-begin);
                if(!tpl_file.empty() && tpl_file[0] != '#') {
                    //struct cfg_tpl* t = new cfg_tpl;
                    struct cfg_tpl_host c;
                    std::string cfg_data;
                    int ret = load(tpl_file.c_str(), cfg_data);
                    assert(ret == 0);
                    std::string host;
                    ret = parse_cfg(cfg_data.c_str(), host, c);
                    assert(ret == 0);

                    LOG(INFO)<<"TPL load tpls for "<<host;
                    maps_tpls.insert(std::pair<std::string, struct cfg_tpl_host>(host, c));
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

#endif

