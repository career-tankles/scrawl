
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

using namespace std;

#include "cJSON.h"
#include "HttpParser.h"
#include "io_ops.h"
#include "utils.h"
#include "cfg_tpl.h"
#include "pugixml_macro.h"

void print(std::map<std::string, std::string>& headers) {
    std::map<std::string, std::string>::iterator iter = headers.begin();
    for(; iter!=headers.end(); iter++) {
        std::cout<<"=================="<<iter->first<<" -> "<<iter->second<<std::endl;
    }
}

void field_add(std::map<std::string, std::string>& extra_fields, std::string key, std::string& value) {
    extra_fields.insert(std::pair<std::string, std::string>(key, value));
}


// 1.解析Headers
int _extract_headers_(std::string& host, const char* headers_data, std::vector<std::string>& need_headers, int records, std::map<std::string, std::string>* out_headers=NULL) {
    std::map<std::string, std::string> parsedheaders;
    HttpParser parser;
    int ret = parser.parse(headers_data, strlen(headers_data), &parsedheaders, NULL);
    //print(headers);
    if(ret == 0) {
        for(int i=0; i<need_headers.size(); i++) {
            if(parsedheaders.count(need_headers[i]) <= 0) {  // redirect页面
                LOG(ERROR)<<"EXTRACTOR header not exist "<<need_headers[i];
                continue;
            } 
            LOG(INFO)<<"EXTRACTOR header "<<records<<" "<<i<<" -- "<<need_headers[i]<<" -> "<<parsedheaders[need_headers[i]];
            if(out_headers)
                out_headers->insert(std::pair<std::string, std::string>(need_headers[i], parsedheaders[need_headers[i]]));
        }
        if(out_headers && out_headers->size() > 0) {
            return 0;
        }
        return 1;
    } else {
        LOG(INFO)<<"EXTRACTOR HttpParser parse headers failed "<<ret;
        return -1;
    }
}

int _extract_http_page_(std::string& host, pugi::xml_document& doc, struct cfg_tpl_chain& c, std::string& json_str, int records, std::map<std::string, std::string>* extra_fields=NULL) {

    pugi::xpath_node_set nodes = doc.select_nodes(c._nodeset_xpath_.c_str());
    if(nodes.size() <= 0) {
        LOG(INFO)<<"EXTRACTOR nodes_set is empty "<<c._nodeset_xpath_;
        return -1;
    }

    LOG(INFO)<<"EXTRACTOR nodeset xpath="<<c._nodeset_xpath_<<" size="<<nodes.size();
    cJSON* jnew_results = cJSON_CreateArray();
    for(int i=0; i<nodes.size(); i++) {
        pugi::xml_node node = nodes[i].node();
        //node.print(std::cout);
        std::string key;
        for(int k=0; k<c._keys_.size(); k++) {
            pugi::xpath_query query_name(c._keys_[k].c_str());
            key = query_name.evaluate_string(node);
            LOG(INFO)<<"EXTRACTOR key "<<c._keys_[k]<<" "<<key;
            if(key.empty()) continue;
            if(c.results.count(key) > 0) break;
        }

        cJSON* jnew_result = cJSON_CreateObject();
        if(key.empty() || c.results.count(key) <= 0) {
            std::string s = key + " handler not set!";
            LOG(WARNING)<<"EXTRACTOR "<<records<<" "<<i<<" -- "<<s;
            cJSON_AddStringToObject(jnew_result, "invalid", s.c_str()); 
            cJSON_AddItemToArray(jnew_results, jnew_result);
            continue;
        }
    
        // 遍历结果模板的每个field
        struct cfg_tpl_item& tpl_item = c.results[key];
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
    
            jfield_type = cJSON_GetObjectItem(next, "type");
            if(jfield_type)
                field_type = jfield_type->valuestring;
    
            jfield_xpath = cJSON_GetObjectItem(next, "xpath");
            if(jfield_xpath)
                field_xpath = jfield_xpath->valuestring;
    
            if(field_xpath.empty()) {
                LOG(INFO)<<"EXTRACTOR "<<records<<" "<<i<<" -- "<<key<<" "<<field_name<<"'s field_xpath is empty"<<std::endl;
                goto Next;
            }
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
    } // endof of for

    if(cJSON_GetArraySize(jnew_results) > 0) {
        cJSON* jnew_root = cJSON_CreateObject();
        cJSON_AddStringToObject(jnew_root, "_name_", c._name_.c_str());
        cJSON_AddStringToObject(jnew_root, "_type_", c._type_.c_str());
        cJSON_AddStringToObject(jnew_root, "host", host.c_str());
        if(extra_fields) {
            std::map<std::string, std::string>::iterator iter = extra_fields->begin();
            for(; iter!=extra_fields->end(); iter++) {
                cJSON_AddStringToObject(jnew_root, iter->first.c_str(), iter->second.c_str());
            }
        }
        cJSON_AddItemToObject(jnew_root, "results", jnew_results);
        char* tmp_out = cJSON_PrintUnformatted(jnew_root);
        json_str = tmp_out;
        free(tmp_out);
        cJSON_Delete(jnew_root);
        return 0;
    } else {
        cJSON_Delete(jnew_results);
        return -3;
    }
}

// 根据c解析模板解析http_data网页数据，生成JSON数据json_str
// @return 0: success  <0: failed
int _extract_data_(std::string host, const char* resp_headers, const char* resp_body, struct cfg_tpl_host& c, std::string& json_str, int records, std::map<std::string, std::string>* extra_fields=NULL) {
    int ret = 0;

    bool has_doc_parsed = false;
    pugi::xml_document doc;

    for(int i=0; i<c.chains.size(); i++) {
        struct cfg_tpl_chain& tpl_chain = c.chains[i];
        if(tpl_chain._type_ == "HEADER") {
            std::map<std::string, std::string> out_headers;
            ret = _extract_headers_(host, resp_headers, tpl_chain._keys_, records, &out_headers);
            if(ret == 0) {
                cJSON* jnew_root = cJSON_CreateObject();
                cJSON_AddStringToObject(jnew_root, "_name_", tpl_chain._name_.c_str());
                cJSON_AddStringToObject(jnew_root, "_type_", tpl_chain._type_.c_str());
                cJSON_AddStringToObject(jnew_root, "host", host.c_str());
                if(extra_fields) {
                    std::map<std::string, std::string>::iterator iter = extra_fields->begin();
                    for(; iter!=extra_fields->end(); iter++) {
                        cJSON_AddStringToObject(jnew_root, iter->first.c_str(), iter->second.c_str());
                    }
                }
                
                cJSON* jnew_results = cJSON_CreateArray();
                std::map<std::string, std::string>::iterator iter = out_headers.begin();
                for(; iter!=out_headers.end(); iter++) {
                    cJSON* jresult = cJSON_CreateObject();
                    cJSON_AddStringToObject(jresult, iter->first.c_str(), iter->second.c_str());
                    cJSON_AddItemToArray(jnew_results, jresult);
                }
                cJSON_AddItemToObject(jnew_root, "results", jnew_results);
                char* tmp_out = cJSON_PrintUnformatted(jnew_root);
                json_str = tmp_out;
                free(tmp_out);
                cJSON_Delete(jnew_root);

                LOG(ERROR)<<"EXTRACTOR _extract_headers_ finish "<<tpl_chain._name_;
                break;
            }
            // TODO:xxxx
        } else if (tpl_chain._type_ == "BODY") {
            if(!has_doc_parsed) {
                pugi::xml_parse_result parse_res = doc.load(resp_body);
                if(!parse_res) {
                    LOG(ERROR)<<"xml_document load error: "<<parse_res.description()<<" "<<parse_res.offset<<" ";
                    return -1;
                }
                has_doc_parsed = true;
            }
            ret = _extract_http_page_(host, doc, tpl_chain, json_str, records, extra_fields);
            if(ret == 0) {
                LOG(ERROR)<<"EXTRACTOR _extract_http_page_ finish "<<tpl_chain._name_<<" "<<json_str;
                break;
            }
        } else {
            LOG(ERROR)<<"AAAAAAAAAAA";
        }
    }
    return ret;
}

// 根据c解析模板解析http_data网页数据，生成JSON数据json_str
// @return 0: success  <0: failed
int extract_http_page_json(cJSON* obj, struct cfg_tpl_host& tpl_host, std::string& json_str) {

    int ret = -1;

    static int records = 0;
    records ++;
   
    assert(obj);

    cJSON* jinfo = cJSON_GetObjectItem(obj, "info");
    assert(jinfo);
    cJSON* jurl = cJSON_GetObjectItem(jinfo, "url");
    assert(jurl);
    cJSON* jhost = cJSON_GetObjectItem(jinfo, "host");
    assert(jhost);
    cJSON* juserdata = cJSON_GetObjectItem(jinfo, "userdata");

    std::string host = jhost->valuestring;
    std::string url = jurl->valuestring;
    std::string userdata;
    if(juserdata)
        userdata = juserdata->valuestring;

    LOG(INFO)<<"EXTRATOR extract data from "<<url<<" userdata: "<<userdata;

    cJSON* jheaders = cJSON_GetObjectItem(obj, "headers");
    cJSON* jbody = cJSON_GetObjectItem(obj, "body");
    assert(jheaders && jheaders->valuestring);
    assert(jbody && jbody->valuestring);

    const char* resp_headers = jheaders->valuestring;
    const char* resp_body = jbody->valuestring;

    std::map<std::string, std::string> extra_fields;
    field_add(extra_fields, "url", url);
    field_add(extra_fields, "userdata", userdata);
    
    ret = _extract_data_(host, resp_headers, resp_body, tpl_host, json_str, records, &extra_fields);
    if(ret == 0) {
        LOG(INFO)<<"EXTRATOR _extract_data_ "<<json_str;
        //parseSearchListJson(json_str.c_str());
    } else {
        LOG(INFO)<<"EXTRATOR extract search_list failed";
    } 
    
    return ret;
}

int parse_http_pages(std::string input_json_file, std::map<std::string, struct cfg_tpl_host>& maps_tpls, std::string output_file) {
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
            cJSON* obj = cJSON_ParseWithOpts(p, &return_parse_end, 0); 
            if(obj == NULL) {
                LOG(INFO)<<"EXTRACTOR parse "<<records<<" records";
                break; 
            }
            
            cJSON* jinfo = cJSON_GetObjectItem(obj, "info");
            if(jinfo == NULL) {
                char* out = cJSON_Print(obj);
                LOG(ERROR)<<"EXTRACTOR jinfo is null, data: "<<out;
                free(out);
                break;
            }
            cJSON* jurl = cJSON_GetObjectItem(jinfo, "url");
            assert(jurl);
            cJSON* jhost = cJSON_GetObjectItem(jinfo, "host");
            assert(jhost);

            std::string host = jhost->valuestring;
            std::string url = jurl->valuestring;

            records++;

            if(maps_tpls.count(host) > 0) {
                std::string return_json_str;
                struct cfg_tpl_host& c = maps_tpls[host];
                int ret = extract_http_page_json(obj, c, return_json_str) ;
                LOG(INFO)<<"AAAAAAAAAAAAA: "<<ret<<" ";
                if(ret == 0) {
                    io_append(outfd, return_json_str);
                    io_append(outfd, "\n", 1);
                }
            } else {
                LOG(INFO)<<"EXTRACTOR parse template for "<<host<<" not exist";
            }
            cJSON_Delete(obj);
            assert(return_parse_end != NULL);   //
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

DEFINE_string(EXTRACTOR_input_tpls_file, "tpls.conf", "");
DEFINE_string(EXTRACTOR_input_format, "JSON", "");
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

    // 解析数据
    if(FLAGS_EXTRACTOR_input_format == "JSON") {
        parse_http_pages(FLAGS_EXTRACTOR_input_file, maps_tpls, FLAGS_EXTRACTOR_output_file);
    }

    return 0;
}

