#ifndef __URI_H__
#define __URI_H__

#include <string>
#include "url-parser.h"

class URI2
{
public:
    URI2()
      :isvalid_(false)
    {
    }
    URI2(std::string url) 
    {
        parse(url);
    }
    URI2(std::string scheme, std::string host, std::string path, unsigned short port=80)
      : scheme_(scheme), host_(host), path_(path), port_(port), isvalid_(true)
    {
        if(port == 0) {
            url_ = scheme_ + "://" + host_ + path_;
        } else {
            char buf[10];
            sprintf(buf, ":%d", port_);
            url_ = scheme_ + "://" + host_ + std::string(buf) + path_;
        }
    }

    ~URI2() {}

    bool parse(std::string url) {
        struct parsed_url* purl = parse_url(url.c_str());
        if(purl == NULL) {
            isvalid_ = false;
        } else {
            isvalid_ = true;

            url_ = url;
            if(purl->scheme)
                scheme_ = purl->scheme;
            if(purl->host)
                host_ = purl->host;
            if(purl->port)
                port_ = atoi(purl->port);
            if(purl->path)
                path_ = purl->path;
            if(purl->query)
                query_ = purl->query;
            if(purl->fragment)
                fragment = purl->fragment;

            parsed_url_free(purl);
            purl = NULL;
        }
        
        return isvalid_;
    }

    inline bool isvalid() { return isvalid_; }
    inline std::string& url() { return url_; }
    inline std::string& host() { return host_; }
    inline std::string& path_only() { return path_; }
    inline std::string& scheme() { return scheme_; }
    inline unsigned short port() { return port_; }
    inline std::string path() { 
        std::string totalpath = path_;
        if(!query_.empty())
            path_ += "?" + query_;

        return totalpath;
    }

private:
    std::string url_;
    std::string scheme_;
    std::string host_;
    std::string path_;
    std::string query_;
    std::string fragment_;
    unsigned short port_;
    bool isvalid_;
};

#endif

