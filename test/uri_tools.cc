#include <assert.h>
#include <iostream>
#include <string>

#include "uri.h"
#include "URI.h"
#include "URI2.h"

int main(int argc, char** argv)
{
    std::string url = "http://m.baidu.com/s?word=xxxx";
    if(argc > 1)
        url = argv[1];

    try{
        std::cout<<url<<"\nuripp::uri:"<<std::endl;
        uripp::uri u(url);
        std::cout<<u<<std::endl;
        std::cout<<u.scheme().string()<<std::endl;
        std::cout<<u.authority()<<std::endl;
        std::cout<<u.authority().host()<<std::endl;
        std::cout<<u.path()<<std::endl;
        std::cout<<u.query()<<std::endl;
        std::cout<<u.path().absolute()<<std::endl;
     }catch(std::exception& e)
    {
        std::cout<<"exception: "<<url<<" "<<e.what()<<std::endl;
    }
 
    try{
        std::cout<<"\nURI:"<<std::endl;
        URI uri(url);
        std::cout<<uri.url()<<std::endl;
        std::cout<<uri.scheme()<<std::endl;
        std::cout<<uri.host()<<std::endl;
        std::cout<<uri.port()<<std::endl;
        std::cout<<uri.path()<<std::endl;

        URI uri2("http", uri.host(), uri.path(), uri.port());
        std::cout<<"url="<<uri2.url()<<std::endl;

     }catch(std::exception& e)
    {
        std::cout<<"exception: "<<url<<" "<<e.what()<<std::endl;
    }
 
    try{
        std::cout<<"\nURI2:"<<std::endl;
        URI2 uri2(url);
        std::cout<<uri2.url()<<std::endl;
        std::cout<<uri2.scheme()<<std::endl;
        std::cout<<uri2.host()<<std::endl;
        std::cout<<uri2.port()<<std::endl;
        std::cout<<uri2.path()<<std::endl;
        std::cout<<uri2.query()<<std::endl;

        URI2 uri3("http", uri2.host(), uri2.path(), uri2.port());
        std::cout<<"url="<<uri3.url()<<std::endl;
    

    }catch(std::exception& e)
    {
        std::cout<<"exception: "<<url<<" "<<e.what()<<std::endl;
    }
 
    return 0;
}
