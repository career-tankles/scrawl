#ifndef _UTILS_H_
#define _UTILS_H_

static void splitBySpace(const char* s, std::vector<std::string>& v) {
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


static void splitByTab(const char* s, std::vector<std::string>& v) {
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

static void splitByChar(const char* s, std::vector<std::string>& v, char delimit=' ') {
    if(!s) return ;

    std::cout<<"["<<s<<"]"<<std::endl;
    const char* start = NULL;
    const char* p = s;
    while(p && *p != '\0') {
        while(p && *p != '\0' && (*p == delimit )) p++; // 第一个非空字符
        start = p;
        while(p && *p != '\0' && *p != delimit) p++; // 起始非空字符
        if(start && p && *p != '\0' ) { 
            v.push_back(std::string(start, p-start));
        } else if(start) {
            v.push_back(std::string(start));
        }   
    }   
}

#endif

