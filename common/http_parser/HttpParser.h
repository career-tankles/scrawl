#ifndef _HTTP_PARSER_H_
#define _HTTP_PARSER_H_

#include <string>
#include <map>

#include "http_parser.h"

struct message;
class HttpParser
{
public:
    HttpParser();
    ~HttpParser();

    int parse(std::string& html_raw_data, std::string* headers=NULL, std::string* body=NULL);
    int parse(std::string& html_raw_data, std::map<std::string, std::string>* headers=NULL, std::string* body=NULL);

private:
    int init();
    int reset();

private:
    http_parser* http_parser_;
    struct message* http_message_;
};

#endif

