#ifndef _HTTP_PARSER_H_
#define _HTTP_PARSER_H_

#include <sys/types.h>
#include <string>
#include <map>

#include "http_parser.h"

struct message;
class HttpParser
{
public:
    HttpParser();
    ~HttpParser();

    int parse(const char* html_raw_data, size_t data_size, std::string* headers=NULL, std::string* body=NULL, size_t* parsed_len=NULL);
    int parse(std::string& html_raw_data, std::string* headers=NULL, std::string* body=NULL, size_t* parsed_len=NULL);

    int parse(const char* html_raw_data, size_t data_size, std::map<std::string, std::string>* headers=NULL, std::string* body=NULL, size_t* parsed_len=NULL);
    int parse(std::string& html_raw_data, std::map<std::string, std::string>* headers=NULL, std::string* body=NULL, size_t* parsed_len=NULL);

private:
    int init();
    int reset();

private:
    http_parser* http_parser_;
    struct message* http_message_;
};

#endif

