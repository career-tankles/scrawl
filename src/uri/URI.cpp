#include "URI.h"

#include <string>
#include <stdio.h>

#include "uri.h"

URI::URI(std::string scheme, std::string host, std::string path, unsigned short port) 
  : scheme_(scheme), host_(host), path_(path), port_(port)
{
    if(port == 0 || port == 80) {
        url_ = scheme_ + "://" + host_ + path_;
    } else {
        char buf[10];
        sprintf(buf, ":%d", port_);
        url_ = scheme_ + "://" + host_ + std::string(buf) + path_;
    }
}

URI::URI(std::string url) {
    parse(url);
}

int URI::parse(std::string url) {
    uripp::uri u(url);
    url_ = u.encoding();
    scheme_ = u.scheme().string();
    host_ = u.authority().host();
    port_ = u.authority().port();
    if(port_ == 0)
        port_ = 80;
    path_ = u.path().encoding();
    if(!u.query().empty())
        path_ += "?" + u.query().encoding();

    return 0;
}

