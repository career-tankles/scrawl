#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <iostream>
#include <stdio.h>

#include "HttpParser.h"

void print(std::map<std::string, std::string>& m) {
    std::map<std::string, std::string>::iterator iter = m.begin();
    for(; iter!=m.end(); iter++)
        std::cout<<iter->first<<": "<<iter->second<<std::endl;
}

int main(int argc, char** argv)
{
    const char* filename = "response.txt";
    if(argc == 2)
        filename = argv[1];

    char data[1024*1024];
    int fd = open(filename, O_RDONLY);
    int len = read(fd, data, sizeof(data));

    std::string html_data(data, len);
    std::string headers, body;
    HttpParser parser;
    parser.parse(html_data, &headers, &body);

    //std::cout<<body<<std::endl<<std::endl;
    std::cout<<headers<<std::endl;

    getchar();

    //std::string parsed_data = headers + "\r\n" + body;
    std::string parsed_data = headers + body;

    std::map<std::string, std::string> headers_map;
    parser.parse(parsed_data, &headers_map, NULL);

    print(headers_map);
    
}
