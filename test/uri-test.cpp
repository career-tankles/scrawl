#include <assert.h>

#include "uri.h"

int main()
{
    std::string url = "http://m.baidu.com/s?word=xxxx";
    if(argc > 1)
        url = argv[1];

    uripp::uri u(url);
    std::cout<<u<<std::endl;
    std::cout<<u.scheme().string()<<std::endl;
    std::cout<<u.authority()<<std::endl;
    std::cout<<u.authority().host()<<std::endl;
    std::cout<<u.path()<<std::endl;
    std::cout<<u.query()<<std::endl;
    std::cout<<u.path().absolute()<<std::endl;
    uripp::path::const_iterator it = u.path().begin();
    assert(*it == "a" && *++it == "b" && *++it == "c" && ++it == u.path().end());
    int id;
    std::string s;
    bool is_null;
    assert(u.query().find("id", id, is_null) && id == 12345 && !is_null);
    assert(u.query().find("s", s, is_null) && s == "x y" && !is_null);
    assert(!u.query().find("foo", s, is_null));
 
    return 0;
}
