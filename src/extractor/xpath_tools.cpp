#include "pugixml.hpp"

#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <time.h>
#include <vector>
#include <fcntl.h>
#include <string.h>
#include <iostream>
#include <string>
#include <sstream>
#include <algorithm>

using namespace std;

#define NODE_EMPTY(_node_) _node_.empty()

#define NODE_SERIALIZATION_string(_node_, _var_) \
        std::string _var_;          \
        if(!NODE_EMPTY(_node_))      \
        {std::ostringstream ostr; _node_.print(ostr); _var_ = ostr.str();}

#define NODE_HAS_ATTR(_node_, _name_, _var_)        \
        bool _var_ = false;                         \
        if(!NODE_EMPTY(_node_))                     \
            if(!_node_.attribute(_name_).empty())   \
                _var_ = true;


#define NODE_ATTR_VALUE(_node_, _name_, _var_, _default_) \
            const pugi::char_t* _var_ = _default_; \
            if(!NODE_EMPTY(_node_)) {\
                pugi::xml_attribute _node_attr_ = _node_.attribute(_name_);\
                if(!_node_attr_.empty()) _var_ = _node_attr_.value(); \
            }

#define NODE_ATTR_VALUE_string(_node_, _name_, _var_, _default_) \
            std::string _var_ = _default_; \
            if(!NODE_EMPTY(_node_)) {\
                pugi::xml_attribute _node_attr_ = _node_.attribute(_name_);\
                if(!_node_attr_.empty()) _var_ = _node_attr_.value(); \
            }


#define NODE_VALUE(_node_, _var_)   \
        std::string _var_;          \
        if(!NODE_EMPTY(_node_))     \
        { _var_ = _node_.value(); }

#define NODE_CHILDREN_RICHTEXT(_node_, _var_) \
        std::string _var_;  \
        for (pugi::xml_node ic = _node_.first_child(); ic; ic = ic.next_sibling()) {    \
            if(ic.type() == pugi::node_element) {   \
                NODE_SERIALIZATION_string(ic, _tmp_str_);   \
                _var_ += _tmp_str_; \
            } else if(ic.type() == pugi::node_pcdata || ic.type() == pugi::node_cdata) \
                _var_ += ic.value();  \
        }

#define NODE_CHILD(_node_, _name_, _var_) \
        pugi::xml_node _var_ = _node_.child(_name_);

#define NODE_FIRST_CHILD(_node_, _name_, _var_) \
        pugi::xml_node _var_ ;                  \
        if(!NODE_EMPTY(_node_)) {               \
            pugi::xml_node _tmp_ = _node_.first_child(); \
            while(!NODE_EMPTY(_tmp_)) {                  \
                std::string cur_name = _tmp_.name();     \
                if(cur_name == _name_) break;            \
                _tmp_ = _tmp_.next_sibling();           \
            }                                            \
            if(!NODE_EMPTY(_tmp_)) _var_ = _tmp_;        \
        }

#define NODE_NEXT(_node_, _name_, _node_next_)  \
        pugi::xml_node _node_next_;             \
        if(!NODE_EMPTY(_node_)) {               \
            pugi::xml_node _tmp_ = _node_.next_sibling();   \
            while(!NODE_EMPTY(_tmp_)) {                     \
                std::string cur_name = _tmp_.name();        \
                if(cur_name == _name_) break;               \
                _tmp_ = _tmp_.next_sibling();               \
            }                                               \
            if(!NODE_EMPTY(_tmp_)) _node_next_ = _tmp_;     \
        }

#define NODE_NEXT_SIBLING(_node_, _node_next_)  \
        pugi::xml_node _node_next_;             \
        if(!NODE_EMPTY(_node_))                 \
            _node_next_ =  _node_.next_sibling();

#define NODE_PRE_SIBLING(_node_, _node_pre_)  \
        pugi::xml_node _node_pre_ = _node_.previous_sibling();

#define NODE_CHILD_WITH_NAME_CLASS(_node_, _name_, _class_, _var_) \
        pugi::xml_node _var_ ;                           \
        if(!NODE_EMPTY(_node_)) {                        \
            pugi::xml_node _tmp_ = _node_.first_child(); \
            while(!NODE_EMPTY(_tmp_)) {                  \
                std::string cur_name = _tmp_.name();     \
                NODE_ATTR_VALUE_string(_tmp_, "class", _class_var_, "");    \
                if(cur_name == _name_ && _class_ == _class_var_) break;     \
                _tmp_ = _tmp_.next_sibling();            \
            }                                            \
            if(!NODE_EMPTY(_tmp_)) _var_ = _tmp_;        \
        }

#
#define NODE_FOREACH(_var_, _node_set_) \
        for(int i=0; i<_node_set_.size(); i++) {\
            pugi::xml_node _var_ = _node_set_[i].node()

#define NODE_FOREACH_END() \
        }

#define NODE_SET_XPATH(_doc_, _xpath_, _var_)   \
    pugi::xpath_node_set _var_;                 \
    {pugi::xpath_query _xpath_query_(_xpath_);  \
     _var_ = _xpath_query_.evaluate_node_set(doc);}

#define AUTO_NODE_BY_XPATH(_root_, _xpath_, _var_)  \
          _var_ = _root_.select_single_node(_xpath_).node()

#define AUTO_NODE_RICHTEXT_BY_XPATH(_node_, _xpath_, _var_) \
        do {pugi::xml_node _tmp_;                           \
            AUTO_NODE_BY_XPATH(_node_, _xpath_, _tmp_);     \
            if(NODE_EMPTY(_node_)) break;                  \
            for (pugi::xml_node ic = _tmp_.first_child(); ic; ic = ic.next_sibling()) {    \
                if(ic.type() == pugi::node_element) {   \
                    NODE_SERIALIZATION_string(ic, _tmp_str_);   \
                    _var_ += _tmp_str_; \
                } else if(ic.type() == pugi::node_pcdata || ic.type() == pugi::node_cdata) \
                    _var_ += ic.value();  \
            }\
        }while(0)

#define AUTO_NODE_RICHTEXT_BY_XPATH2(_node_, _xpath_, _var_, _need_tags_v_) \
        do {pugi::xml_node _tmp_;                           \
            AUTO_NODE_BY_XPATH(_node_, _xpath_, _tmp_);     \
            if(NODE_EMPTY(_node_)) break;                  \
            for (pugi::xml_node ic = _tmp_.first_child(); ic; ic = ic.next_sibling()) {    \
                std::vector<std::string>::iterator iter = std::find(_need_tags_v_.begin(), _need_tags_v_.end(), std::string(ic.name())); \
                if(ic.type() == pugi::node_element && std::find(_need_tags_v_.begin(), _need_tags_v_.end(), ic.name()) !=_need_tags_v_.end()) {   \
                    NODE_SERIALIZATION_string(ic, _tmp_str_);   \
                    _var_ += _tmp_str_; \
                } else if(ic.type() == pugi::node_pcdata || ic.type() == pugi::node_cdata) { \
                    _var_ += ic.value();  \
                } \
            }\
        }while(0)


#define AUTO_NODE_ATTR_VALUE_string(_node_, _name_, _var_, _default_) \
            do{_var_ = _default_; \
            if(!NODE_EMPTY(_node_)) {\
                pugi::xml_attribute _node_attr_ = _node_.attribute(_name_);\
                if(!_node_attr_.empty()) _var_ = _node_attr_.value(); \
            }}while(0)


#define AUTO_NODE_ATTR_BY_XPATH(_root_, _xpath_, _var_) \
        do{ pugi::xml_attribute _attr_tmp_ = _root_.select_single_node(_xpath_).attribute();   \
          if(!_attr_tmp_.empty()) \
            _var_ = _attr_tmp_.value(); \
        }while(0)

    /***
    {
        res XPATH "/html/body/div/div/div[@class='resitem' and @srcid='81']"
        href XPATH "/html/body/div/div/div[@class='resitem' and @srcid='81']/a/@href" 
        a   SUB_XPATH res "a"
        href ATTR  a 
        richtext RICHTEXT a
    }
    ***/
#include <map>
#include <assert.h>
#include "cJSON.h"
struct result_tpl {
    std::string _key_ ;
    cJSON* jnode;
    result_tpl(): _key_(""), jnode(NULL) {}
};

struct cfg_tpl {
    std::string host;
    std::string xpath;
    std::string _key_;
    cJSON* jroot;

    std::map<std::string, struct result_tpl> results;

    cfg_tpl(): host(""), xpath(""), _key_(""), jroot(NULL) {}
    ~cfg_tpl() {
        if(jroot) {
            cJSON_Delete(jroot);
            jroot = NULL;
        }
    }
};

const char* cfg_str = 
    "{"
    "    \"host\": \"m.baidu.com\","
    "    \"xpath\": \"/html/body/div/div/div[@class='resitem']\","
    "    \"_key_\": \"concat(@class, '-', @srcid, '-', @tpl)\","
    "    \"results\": ["
    "        {"
    "            \"_key_\": \"resitem--\","
    "            \"srcid\": {\"type\":\"ATTR\", \"xpath\":\"@srcid\"},"
    "            \"href\": {\"type\":\"ATTR\", \"xpath\":\"a/@href\"},"
    "            \"anchor\": {\"type\":\"RICHTEXT\", \"xpath\":\"a\"}"
    "        },"
    "        {"
    "            \"_key_\": \"resitem-81-baikeimg\","
    "            \"srcid\": {\"type\":\"ATTR\", \"xpath\":\"@srcid\"}"
    "        },"
    "        {"
    "            \"_key_\": \"resitem-map-map\","
    "            \"srcid\": {\"type\":\"ATTR\", \"xpath\":\"@srcid\"}"
    "        }"
 
    "    ]"
    "}";
 
int parse_cfg(cJSON* jroot, struct cfg_tpl* tpl) {
       if(!jroot){
        printf("jroot is null\n"); 
        return -1;
    }
    cJSON* jhost = cJSON_GetObjectItem(jroot, "host");
    if(jhost == NULL ) return -2;
    printf("host:%s\n", jhost->valuestring); 
  
    cJSON* jxpath = cJSON_GetObjectItem(jroot, "xpath");
    if(jxpath == NULL ) return -3;
    printf("xpath:%s\n", jxpath->valuestring); 

    cJSON* j_key_= cJSON_GetObjectItem(jroot, "_key_");
    if(j_key_== NULL ) return -4;
    printf("_key_:%s\n", j_key_->valuestring); 

    if(tpl == NULL)
        tpl = new cfg_tpl;

    assert(tpl);

    tpl->jroot = jroot;
    tpl->host = jhost->valuestring;
    tpl->xpath = jxpath->valuestring;
    tpl->_key_ = j_key_->valuestring;

    cJSON* jresults = cJSON_GetObjectItem(jroot, "results");
    for (int i = 0 ; jresults && i < cJSON_GetArraySize(jresults) ; i++) {
        cJSON* jresult = cJSON_GetArrayItem(jresults, i);

        cJSON* j_key_= cJSON_GetObjectItem(jresult, "_key_");
        if(j_key_== NULL ) return -5;
        printf("   _key_:%s\n", j_key_->valuestring); 

        result_tpl result;
        result._key_ = j_key_->valuestring;
        result.jnode = jresult;
        tpl->results.insert(std::pair<std::string, struct result_tpl>(result._key_, result));
    }
    return 0;
}

int parse_cfg(const char* json_str, struct cfg_tpl* tpl) {
    cJSON* jroot = cJSON_Parse(json_str);
    if(jroot == NULL) {
        std::cout<<"Error:"<<((cJSON_GetErrorPtr()-80)>json_str?(cJSON_GetErrorPtr()-80):json_str)<<std::endl;
        return -1;
    }
    return parse_cfg(jroot, tpl);
}

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

void print(std::vector<std::string>& v) {
    for(int i=0; i<v.size(); i++)
        cout<<"["<<v[i]<<"]"<<endl;
    cout<<endl;
}

void splitBySpace(const char* s, std::vector<std::string>& v) {
    if(!s) return ;

    std::cout<<"["<<s<<"]"<<std::endl;
    const char* start = NULL;
    const char* p = s;
    while(p && *p != '\0') {
        while(p && *p != '\0' && (*p == ' ' || *p == '\t' || *p == '\n')) p++; // 第一个非空字符
        start = p;
        while(p && *p != '\0' && *p != ' ' && *p != '\t' && *p != '\n') p++; // 起始非空字符
        if(start && p && *p != '\0' ) { 
            v.push_back(std::string(start, p-start));
        } else if(start) {
            v.push_back(std::string(start));
        }   
    }   
}
int load_tpls(std::string file, std::map<std::string, struct cfg_tpl*>& maps_tpls) {
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
                std::string line = std::string(begin, p-begin);
                if(!line.empty() && line[0] != '#') {
                    std::vector<std::string> v ;
                    splitBySpace(line.c_str(), v);
                    print(v);
                    if(v.size() == 2) {
                        struct cfg_tpl* t = new cfg_tpl;
                        struct cfg_tpl& c = *t;
                        std::string cfg_data;
                        int ret = load(v[1].c_str(), cfg_data);
                        assert(ret == 0);
                        ret = parse_cfg(cfg_data.c_str(), &c);
                        assert(ret == 0);

                        std::cout<<"conf: "<<v[0]<<" "<<v[1]<<std::endl;
                        maps_tpls.insert(std::pair<std::string, struct cfg_tpl*>(v[0], t));
                    }
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


// 根据c解析模板解析http_data网页数据，生成JSON数据json_str
// @return 0: success  <0: failed
int extract_http_page(const char* html_data, struct cfg_tpl& c, std::string& json_str) {
   
    std::cout<<"extract_http_page AAAAAAAAAAAAAAAAAAA"<<std::endl;

    pugi::xml_document doc;
    pugi::xml_parse_result parse_res = doc.load(html_data);
    if(!parse_res) {
        std::cout<<"xml_document load error: "<<parse_res.description()<<" "<<parse_res.offset<<std::endl;
        return -1;
    }

    cJSON* jnew_root = cJSON_CreateObject();

    cJSON_AddStringToObject(jnew_root, "md5", "md5(保定)");
    cJSON_AddStringToObject(jnew_root, "host", "http://m.baidu.com");
    cJSON_AddStringToObject(jnew_root, "query", "保定");
    cJSON_AddNumberToObject(jnew_root, "time", time(NULL));
    cJSON_AddStringToObject(jnew_root, "stime", "strftime(time)");

    cJSON* jnew_results = cJSON_CreateArray();


    pugi::xpath_node_set nodes = doc.select_nodes(c.xpath.c_str());
    std::cout<<"c.xpath="<<c.xpath<<" size="<<nodes.size()<<std::endl;
    for(int i=0; i<nodes.size(); i++) {
        pugi::xml_node node = nodes[i].node();
        //node.print(std::cout);
        pugi::xpath_query query_name(c._key_.c_str());
        std::string key = query_name.evaluate_string(node);
        std::cout<<"\nKEY="<<key<<std::endl;

        cJSON* jnew_result = cJSON_CreateObject();

        if(c.results.count(key) == 0) {
            std::string invalid_val = "WARNING: key=" + key + " handler not set!";
            cJSON_AddStringToObject(jnew_result, "invalid", invalid_val.c_str());
            cJSON_AddItemToArray(jnew_results, jnew_result);
            
            continue;
        }
        result_tpl& res_tpl = c.results[key];
        assert(res_tpl.jnode);

        // 遍历结果模板的每个field
        cJSON* next = res_tpl.jnode->child;
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
                std::cout<<"======"<<field_name<<" field_xpath is empty"<<std::endl;
                goto Next;
            }

            if(field_type == "ATTR")
                AUTO_NODE_ATTR_BY_XPATH(node, field_xpath.c_str(), field_value);
            else if(field_type == "RICHTEXT"){
                std::vector<std::string> need_tags_v;
                jfield_need_tags = cJSON_GetObjectItem(next, "need_tags");
                for(int i=0; jfield_need_tags && i < cJSON_GetArraySize(jfield_need_tags); i++) {
                    cJSON* _jneed_tag = cJSON_GetArrayItem(jfield_need_tags, i);
                    need_tags_v.push_back(_jneed_tag->valuestring);
                }
                
                AUTO_NODE_RICHTEXT_BY_XPATH2(node, field_xpath.c_str(), field_value, need_tags_v);
            }
                
            if(field_value.empty()) {
                std::cout<<"======"<<field_name<<" "<<field_type<<" "<<field_xpath<<" field_value=null"<<std::endl;
                goto Next;
            }

            cJSON_AddStringToObject(jnew_result, field_name.c_str(), field_value.c_str());
            cout<<"----- "<<field_name<<" "<<field_type<<" "<<field_xpath<<" "<<field_value<<std::endl;

        Next:
            next = next->next;
    
        } // endof of while(next)
        cJSON_AddItemToArray(jnew_results, jnew_result);

        getchar();
    }

    cJSON_AddItemToObject(jnew_root, "results", jnew_results);
    //std::cout<<cJSON_Print(jnew_root)<<std::endl;
    json_str = cJSON_PrintUnformatted(jnew_root);
    cJSON_Delete(jnew_root);

    return 0;
}

int extract_http_page(std::string& html_data, struct cfg_tpl& c, std::string& json_str) {
    return extract_http_page(html_data.c_str(), c, json_str);
}

void parseJson(const char* file, std::map<std::string, struct cfg_tpl*>& maps_tpls) {
    int fd = open(file, O_RDONLY);
    if(fd == -1) {
        fprintf(stderr, "Error: %s %d-%s\n", file, errno, strerror(errno));
        return ;
    }

    int n = 0, len = 0;
    char buf[1024*1024];
    while((n=read(fd, buf+len, sizeof(buf)-len-1)) > 0) {
        len += n;
        const char* p = buf;
        while(len > 0) {
            const char* return_parse_end = NULL;
            cJSON* obj = cJSON_ParseWithOpts(p, &return_parse_end, 0); // obj3!=NULL情况下，return_parse_end通常不为NULL
            if(obj == NULL)
                break; 
            
            cJSON* jinfo = cJSON_GetObjectItem(obj, "info");
            assert(jinfo);
            cJSON* jurl = cJSON_GetObjectItem(jinfo, "url");
            assert(jurl);
            cJSON* jhost = cJSON_GetObjectItem(jinfo, "host");
            assert(jhost);
            cJSON* jbody = cJSON_GetObjectItem(obj, "body");
            assert(jbody);


            std::cout<<jurl->valuestring<<std::endl;
            //std::cout<<jbody->valuestring<<std::endl;
            
            std::string host = jhost->valuestring;
            std::cout<<"--"<<jhost->valuestring<<"--"<<std::endl;
            
            if(maps_tpls.count(host) > 0) {
                struct cfg_tpl*& c = maps_tpls[host];
                std::string return_json_str("");
                int ret = extract_http_page(jbody->valuestring, *c, return_json_str) ;
                assert(ret == 0);
                if(ret == 0)
                    std::cout<<return_json_str<<std::endl;
            }
            cJSON_Delete(obj);
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

int main()
{

    std::map<std::string, struct cfg_tpl*> maps_tpls;
    int ret = load_tpls("tpls.conf", maps_tpls) ;
    assert(ret == 0);

    if(1) {
        //parseJson("/tmp/spider/data/1", maps_tpls);
        parseJson("/tmp/spider/data/total", maps_tpls);
    }

    if(0) {
        std::string cfg_data ;
        ret = load("tpl.m.baidu.com.conf", cfg_data);
        assert(ret == 0);
    
        std::string html_data ;
        ret = load("a.html", html_data);
        assert(ret == 0);
    
        std::string host = "m.baidu.com";
        if(maps_tpls.count(host)==0)
            std::cout<<"not exit host"<<std::endl;
        else {
            struct cfg_tpl*& c = maps_tpls[host];
    
            std::string return_json_str;
            std::cout<<"extract_http_page"<<std::endl;
            ret = extract_http_page(html_data, *c, return_json_str) ;
            assert(ret == 0);
            
            std::cout<<return_json_str<<std::endl;
        }
    }

    std::map<std::string, struct cfg_tpl*>::iterator iter = maps_tpls.begin();
    for(; iter!=maps_tpls.end(); iter++) {
        struct cfg_tpl* tpl = iter->second ;
        delete tpl;
        tpl = NULL;
    }
}

