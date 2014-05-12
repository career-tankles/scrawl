

#include <stdio.h>
#include <zlib.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <glog/logging.h>
#include <gflags/gflags.h>
#include <map>
#include <string>
#include <iostream>

#include "cJSON.h"
#include "base64.h"
#include "io_ops.h"
#include "cfg_tpl.h"
#include "URI2.h"
#include "pugixml_macro.h"

struct SpiderDataEntity
{
    int errno_;
    std::string optype_;
    size_t pagesize_;
    std::string url_;
    std::string durl_;
    size_t dlts_;
    int httpcode_;
    std::string content_type_;
    std::string opts_;
    std::string type_;  
    std::string content_;

    bool isvalid_ ;
    unsigned int records;
    
    SpiderDataEntity()
        : errno_(0), optype_(""), pagesize_(0), url_("")
        , durl_(""), dlts_(0), httpcode_(0), content_type_("")
        , opts_(""), type_(""), content_(""), isvalid_(false)
        , records(0)
    {}
    void reset() {
        errno_ = 0;
        optype_ = "";
        pagesize_ = 0 ;
        url_ = "";
        durl_ = "";
        dlts_ = 0;
        httpcode_ = 0;
        content_type_ = "";
        opts_ = "";
        type_ = "";
        content_ = "";
        isvalid_ = false;
    }
};

struct cb_param {
    std::map<std::string, struct cfg_tpl_host>* tpls;
    std::string input_file;
    std::string output_file;
    int output_fd;
    SpiderDataEntity entity;
};

// 根据c解析模板解析http_data网页数据，生成JSON数据json_str
// @return 0: success  <0: failed
int _extract_data_(unsigned int records, std::string url, const char* content, std::map<std::string, struct cfg_tpl_host>& tpls, std::string& out_json_str) {

    int ret = 0;

    URI2 uri(url);
    std::string host = uri.host(); // TODO: parse host from url
    if(host.empty()) return -1;
    struct cfg_tpl_host& c = tpls[host];
    fprintf(stderr, "HOST: %s\n", host.c_str());

    pugi::xml_document doc;
    pugi::xml_parse_result parse_res = doc.load(content);
    if(!parse_res) {
        LOG(ERROR)<<"xml_document load error: "<<parse_res.description()<<" "<<parse_res.offset<<" ";
        return -1;
    }
 
    for(int i=0; i<c.chains.size(); i++) {
        struct cfg_tpl_chain& tpl_chain = c.chains[i];
        if (tpl_chain._type_ == "BODY") {
            pugi::xpath_node_set nodes = doc.select_nodes(tpl_chain._nodeset_xpath_.c_str());
            if(nodes.size() <= 0) {
                LOG(INFO)<<"EXTRACTOR nodes_set is empty "<<tpl_chain._nodeset_xpath_;
                continue;
            }
            LOG(INFO)<<"EXTRACTOR nodeset xpath="<<tpl_chain._nodeset_xpath_<<" size="<<nodes.size();
            cJSON* jnew_results = cJSON_CreateArray();
            for(int i=0; i<nodes.size(); i++) {
                pugi::xml_node node = nodes[i].node();
                std::string key;
                for(int k=0; k<tpl_chain._keys_.size(); k++) {
                    pugi::xpath_query query_name(tpl_chain._keys_[k].c_str());
                    key = query_name.evaluate_string(node);
                    LOG(INFO)<<"EXTRACTOR key "<<tpl_chain._keys_[k]<<" "<<key;
                    if(key.empty()) continue;
                    if(tpl_chain.results.count(key) > 0) break;
                }
        
                cJSON* jnew_result = cJSON_CreateObject();
                if(key.empty() || tpl_chain.results.count(key) <= 0) {
                    std::string s = key + " handler not set!";
                    LOG(WARNING)<<"EXTRACTOR "<<records<<" "<<i<<" -- "<<s;
                    cJSON_AddStringToObject(jnew_result, "invalid", s.c_str()); 
                    cJSON_AddItemToArray(jnew_results, jnew_result);
                    continue;
                }
            
                // 遍历结果模板的每个field
                struct cfg_tpl_item& tpl_item = tpl_chain.results[key];
                assert(tpl_item.jnode);
                cJSON* next = tpl_item.jnode->child;
                while(next) {
                    cJSON* jfield_type = NULL;
                    cJSON* jfield_xpath = NULL;
                    cJSON* jfield_need_tags = NULL;
                    cJSON* jfield_need_tag = NULL;
                    std::string field_type;
                    std::string field_xpath;
                    std::string field_value;
                    std::string field_need_tags ;
            
                    std::string field_name = next->string;
                    if(field_name == "_key_" || field_name.empty()) goto Next;
                       
                    jfield_xpath = cJSON_GetObjectItem(next, "_xpath_");
                    if(jfield_xpath)
                        field_xpath = jfield_xpath->valuestring;
            
                    if(field_xpath.empty()) {
                        LOG(INFO)<<"EXTRACTOR "<<records<<" "<<i<<" -- "<<key<<" "<<field_name<<"'s field_xpath is empty"<<std::endl;
                        goto Next;
                    }
 
                    jfield_type = cJSON_GetObjectItem(next, "_type_");
                    if(jfield_type)
                        field_type = jfield_type->valuestring;

                    if(field_type == "EVAL") {
                        pugi::xpath_query query_name(field_xpath.c_str());
                        field_value = query_name.evaluate_string(node);
                    } else if(field_type == "ATTR") {
                        AUTO_NODE_ATTR_BY_XPATH(node, field_xpath.c_str(), field_value);
                    } else if(field_type == "RICHTEXT"){
                        std::vector<std::string> need_tags_v;
                        jfield_need_tags = cJSON_GetObjectItem(next, "need_tags");
                        for(int i=0; jfield_need_tags && i < cJSON_GetArraySize(jfield_need_tags); i++) {
                            cJSON* _jneed_tag = cJSON_GetArrayItem(jfield_need_tags, i);
                            need_tags_v.push_back(_jneed_tag->valuestring);
                        }
                        
                        AUTO_NODE_RICHTEXT_BY_XPATH2(node, field_xpath.c_str(), field_value, need_tags_v);
                    } else if(field_type == "CONSTANT") {
                        // 常量
                        field_value = field_xpath;
                    }

                    if(field_value.empty()) {
                        LOG(WARNING)<<"EXTRACTOR "<<records<<" "<<i<<" -- '"<<key<<"' "<<field_name<<" "<<field_type<<" "<<field_xpath<<" field_value=null";
                        goto Next;
                    }
            
                    cJSON_AddStringToObject(jnew_result, field_name.c_str(), field_value.c_str());
                    LOG(INFO)<<"EXTRACTOR "<<records<<" "<<i<<" -- "<<key<<" "<<field_name<<" "<<field_type<<" "<<field_xpath<<" "<<field_value;
            
                Next:
                    next = next->next;
            
                } // endof of while(next)
                cJSON_AddItemToArray(jnew_results, jnew_result);
            } // endof of for(int i=0; i<nodes.size(); i++) 

            if(cJSON_GetArraySize(jnew_results) > 0) {
                cJSON* jnew_root = cJSON_CreateObject();
                cJSON_AddStringToObject(jnew_root, "_name_", tpl_chain._name_.c_str());
                cJSON_AddStringToObject(jnew_root, "_type_", tpl_chain._type_.c_str());
                cJSON_AddStringToObject(jnew_root, "host", host.c_str());
                cJSON_AddStringToObject(jnew_root, "url", url.c_str());
                cJSON_AddItemToObject(jnew_root, "results", jnew_results);
                std::cout<<cJSON_Print(jnew_root)<<std::endl;
                out_json_str = cJSON_PrintUnformatted(jnew_root);
                //out_json_str = cJSON_Print(jnew_root);
                cJSON_Delete(jnew_root);
                return 0;
            } else {
                cJSON_Delete(jnew_results);
                return -3;
            }
        } else {
            LOG(ERROR)<<"ERROR: Unknown chain.type "<<tpl_chain._type_;
        }
    } // endof for(int i=0; i<tpl_chain.chains.size(); i++) 

    return ret;
}

static void spider_data_parser(char* data, size_t len, void* args = NULL)
{
    if(!data || len <= 0) 
        return ;

    int ret = 0;

    cb_param* p = (cb_param*)args; assert(p);
    std::map<std::string, struct cfg_tpl_host>* tpls = p->tpls;
    assert(tpls);
    assert(p->output_fd != -1);

    p->entity.reset();

    cJSON* jroot = cJSON_Parse(data);
    if(!jroot) {
        fprintf(stderr, "cJSON_Parse failed!\n");
        return ;
    }
    cJSON* joptype = cJSON_GetObjectItem(jroot, "optype");
    cJSON* jerrno = cJSON_GetObjectItem(jroot, "errno");
    cJSON* jurl = cJSON_GetObjectItem(jroot, "url");
    cJSON* jdurl = cJSON_GetObjectItem(jroot, "durl");

    p->entity.isvalid_ = true;
    p->entity.optype_ = joptype->valuestring;
    p->entity.url_ = jurl->valuestring;
    p->entity.durl_ = jdurl->valuestring;
    p->entity.errno_ = jerrno->valueint;

    cJSON* jopts = cJSON_GetObjectItem(jroot, "opts");
    if(jopts) p->entity.opts_ = jopts->valuestring;

    cJSON* jpagesize = cJSON_GetObjectItem(jroot, "pagesize");
    if(jpagesize) p->entity.pagesize_ = jpagesize->valueint;

    cJSON* jdlts= cJSON_GetObjectItem(jroot, "dlts");
    if(jdlts) p->entity.dlts_ = jdlts->valueint;

    cJSON* jhttpcode = cJSON_GetObjectItem(jroot, "httpcode");
    if(jhttpcode) p->entity.httpcode_ = jhttpcode->valueint;

    cJSON* jcontent_type = cJSON_GetObjectItem(jroot, "content_type");
    if(jcontent_type) p->entity.content_type_ = jcontent_type->valuestring;

    cJSON* jvalues = cJSON_GetObjectItem(jroot, "value");
    assert(cJSON_GetArraySize(jvalues) == 1);   // 当前看到的都是只有一个，如果出现多个再处理
    for(int i=0; jvalues && i<cJSON_GetArraySize(jvalues); i++) 
    {
        cJSON* jvalue = cJSON_GetArrayItem(jvalues, i);
        if(jvalue) {
            cJSON* jattrs = cJSON_GetObjectItem(jvalue, "attrs");
            if(jattrs) {
                cJSON* jcontent = cJSON_GetObjectItem(jattrs, "content");
                if(jcontent && jcontent->valuestring) {
                    std::string content = jcontent->valuestring;
                    std::string decoded = base64_decode(content);
                    unsigned long buf_len = decoded.size() * 10;
                    unsigned char* buf = (unsigned char*)malloc(buf_len+1);
                    memset(buf, 0, buf_len+1);
                    ret = uncompress(buf, &buf_len, (unsigned char*)decoded.c_str(), decoded.size()); // TODO: 错误处理
                    if(ret == Z_BUF_ERROR) {
                        LOG(ERROR)<<"uncompress: The buffer dest was not large enough to hold the uncompressed data.";
                    } else if(ret == Z_MEM_ERROR) {
                        LOG(ERROR)<<"uncompress: Insufficient memory.";
                    } else if(ret == Z_DATA_ERROR) {
                        LOG(ERROR)<<"uncompress: The compressed data (referenced by source) was corrupted.";
                    } else if (ret == Z_OK) {
                        p->entity.content_ = std::string((char*)buf, buf_len);
                        //std::cout<<p->entity.content_<<std::endl;
                        fprintf(stderr, "url:%s\n", p->entity.url_.c_str());
                        std::string out_json_str("");
                        ret = _extract_data_(p->entity.records++, p->entity.url_, p->entity.content_.c_str(), *tpls, out_json_str);
                        if(ret == 0 && out_json_str.size() > 0) {
                            io_append(p->output_fd, out_json_str);
                            io_append(p->output_fd, "\n", 1);
                        }
                    } else {
                        LOG(ERROR)<<"uncompress: unknown error.";
                    }
                    free(buf);
                }
            }
        }
    } // endof for(int i=0; jvalues && i<cJSON_GetArraySize(jvalues); i++) 
}

DEFINE_string(EXTRACTOR_input_tpls_file, "tpls.conf", "");
DEFINE_string(EXTRACTOR_input_format, "SPIDER_SERVICE_JSON", "");
DEFINE_string(EXTRACTOR_URL, "", "need when EXTRACTOR_input_format==HTML");
DEFINE_string(EXTRACTOR_input_file, "input.json", "");
DEFINE_string(EXTRACTOR_output_file, "output.json", "");

int main(int argc, char** argv)
{
    google::ParseCommandLineFlags(&argc, &argv, true);
    // Initialize Google's logging library.
    FLAGS_logtostderr = 1;  // 默认打印错误输出
    google::InitGoogleLogging(argv[0]);

    // 加载配置
    std::map<std::string, struct cfg_tpl_host> maps_tpls;
    int ret = load_tpls(FLAGS_EXTRACTOR_input_tpls_file, maps_tpls) ;
    assert(ret == 0);

    int outfd = open(FLAGS_EXTRACTOR_output_file.c_str(), O_WRONLY|O_CREAT, S_IRWXU|S_IRWXG|S_IRWXO);
    if(outfd == -1) {
        fprintf(stderr, "Error: %s %d-%s\n", FLAGS_EXTRACTOR_output_file.c_str(), errno, strerror(errno));
        return -2;
    }

    // 解析数据
    if(FLAGS_EXTRACTOR_input_format == "SPIDER_SERVICE_JSON") {
        cb_param p ;
        p.tpls = &maps_tpls; 
        p.input_file = FLAGS_EXTRACTOR_input_file;
        p.output_file = FLAGS_EXTRACTOR_output_file;
        p.output_fd = outfd;
        load_file(FLAGS_EXTRACTOR_input_file.c_str(), spider_data_parser, (void*)&p);
    } else if(FLAGS_EXTRACTOR_input_format == "HTML") {    // 单个html页面
        if(FLAGS_EXTRACTOR_URL.empty()) {
            LOG(INFO)<<"EXTRACTOR_URL is needed when input_format==HTML";
            return -3;
        }
        std::string content;
        ret = load(FLAGS_EXTRACTOR_input_file.c_str(), content);
        assert(ret == 0);
        std::string out_json_str;
        ret = _extract_data_(0, FLAGS_EXTRACTOR_URL, content.c_str(), maps_tpls, out_json_str);
        std::cout<<out_json_str<<std::endl;
        if(ret == 0 && out_json_str.size() > 0) {
            io_append(outfd, out_json_str);
            io_append(outfd, "\n", 1);
        }
    }

    if(outfd != -1) {
        close(outfd);
    }
}

