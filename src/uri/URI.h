#ifndef __URI_H__
#define __URI_H__

#include <string>

class URI
{
public:
    URI()  {port_ = 80;}
    URI(std::string url);
    URI(std::string scheme, std::string host, std::string path, unsigned short port=80);
    ~URI() {}

    int parse(std::string url) ;

    inline std::string& url() { return url_; }
    inline std::string& host() { return host_; }
    inline std::string& path() { return path_; }
    inline std::string& scheme() { return scheme_; }
    inline unsigned short port() { return port_; }

private:
    std::string url_;
    std::string scheme_;
    std::string host_;
    std::string path_;
    unsigned short port_;
};

#endif

