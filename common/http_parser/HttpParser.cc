
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>

#include "HttpParser.h"

#undef TRUE
#define TRUE 1
#undef FALSE
#define FALSE 0

#define MAX_HEADERS 13
#define MAX_ELEMENT_SIZE 2048
#define MAX_HTTP_HEADER_SIZE 10*1024
#define MAX_HTTP_PAGE_SIZE 1024*1024


static size_t
strlncat(char *dst, size_t len, const char *src, size_t n)
{
  size_t slen;
  size_t dlen;
  size_t rlen;
  size_t ncpy;

  slen = strnlen(src, n);
  dlen = strnlen(dst, len);

  if (dlen < len) {
    rlen = len - dlen;
    ncpy = slen < rlen ? slen : (rlen - 1);
    memcpy(dst + dlen, src, ncpy);
    dst[dlen + ncpy] = '\0';
  }

  assert(len > slen + dlen);
  return slen + dlen;
}

static size_t
strlcat(char *dst, const char *src, size_t len)
{
  return strlncat(dst, len, src, (size_t) -1);
}

static size_t
strlncpy(char *dst, size_t len, const char *src, size_t n)
{
  size_t slen;
  size_t ncpy;

  slen = strnlen(src, n);

  if (len > 0) {
    ncpy = slen < len ? slen : (len - 1);
    memcpy(dst, src, ncpy);
    dst[ncpy] = '\0';
  }

  assert(len > slen);
  return slen;
}

static size_t
strlcpy(char *dst, const char *src, size_t len)
{
  return strlncpy(dst, len, src, (size_t) -1);
}

struct message {
  enum http_method method;
  int status_code;
  char response_status[MAX_ELEMENT_SIZE];

  size_t body_size;
  char body[MAX_HTTP_PAGE_SIZE];

  int num_headers;
  char headers [MAX_HEADERS][2][MAX_ELEMENT_SIZE];

  unsigned short http_major;
  unsigned short http_minor;

  bool b_parser_header;
  bool b_parser_body;
};

static int
message_begin_cb (http_parser *p)
{
  return 0;
}

static int
header_field_cb (http_parser *p, const char *buf, size_t len)
{
  struct message *m = (struct message*)p->data;
  assert(m);

  if(!m->b_parser_header) return 0; // 不解析header

  m->num_headers++;
  strlncat(m->headers[m->num_headers-1][0],
           sizeof(m->headers[m->num_headers-1][0]),
           buf,
           len);
  return 0;
}


static int
header_value_cb (http_parser *p, const char *buf, size_t len)
{
  struct message *m = (struct message*)p->data;
  assert(m);

  if(!m->b_parser_header) return 0; // 不解析header

  strlncat(m->headers[m->num_headers-1][1],
           sizeof(m->headers[m->num_headers-1][1]),
           buf,
           len);
  return 0;
}

static int
response_status_cb (http_parser *p, const char *buf, size_t len)
{
  struct message *m = (struct message*)p->data;
  assert(m);
  if(!m->b_parser_header) return 0; // 不解析header
  strlncat(m->response_status,
           sizeof(m->response_status),
           buf,
           len);
  return 0;
}

static int
count_body_cb (http_parser *p, const char *buf, size_t len)
{
  struct message *m = (struct message*)p->data;
  assert(m);

  if(!m->b_parser_body) return 0;   // 不解析body

  assert(buf);
  strlncat(m->body+m->body_size,
           sizeof(m->body)-m->body_size-1,
           buf,
           len);
  m->body_size += len;
  return 0;
}

static int
headers_complete_cb (http_parser *p)
{
  struct message *m = (struct message*)p->data;
  assert(m);

  if(!m->b_parser_header) return 0; // 不解析header

  m->status_code = p->status_code;
  m->http_major = p->http_major;
  m->http_minor = p->http_minor;

  char buf[100];
  snprintf(buf, sizeof(buf), "HTTP/%d.%d %d %s", m->http_major, m->http_minor, m->status_code, m->response_status);
  strlncpy(m->response_status,
           sizeof(m->response_status),
           buf,
           strlen(buf));
  return 0;
}

static int
message_complete_cb (http_parser *p)
{
  struct message *m = (struct message*)p->data;
  assert(m);


  return 0;
}
/*
struct http_parser_settings {
  http_cb      on_message_begin;
  http_data_cb on_url;
  http_data_cb on_status;
  http_data_cb on_header_field;
  http_data_cb on_header_value;
  http_cb      on_headers_complete;
  http_data_cb on_body;
  http_cb      on_message_complete;
};
*/

static http_parser_settings settings_count_body =
  {message_begin_cb
  ,NULL
  ,response_status_cb
  ,header_field_cb
  ,header_value_cb
  ,headers_complete_cb
  ,count_body_cb
  ,message_complete_cb
  };


HttpParser::HttpParser() {
    init();
}

HttpParser::~HttpParser() {
    if(http_parser_) {
        free(http_parser_);
        http_parser_ = NULL;
    }
    if(http_message_) {
        free(http_message_);
        http_message_ = NULL;
    }
}

int HttpParser::init() {
    http_parser_ = (http_parser*)malloc(sizeof(http_parser));
    http_message_ = (struct message*)malloc(sizeof(struct message));
}

int HttpParser::reset() {
    assert(http_parser_);
    memset((void*)http_parser_, 0, sizeof(http_parser));
    memset((void*)http_message_, 0, sizeof(struct message));
    http_parser_init(http_parser_, HTTP_RESPONSE);
    http_parser_->data = (void*)http_message_;       // 
}

static void _message_to_headers_body(struct message* m, std::string* headers=NULL, std::string* body=NULL) {

    if(!m) return ;

    if(headers) {
        char header_buf[MAX_HTTP_HEADER_SIZE];
    
        int len = 0;
        len += snprintf(header_buf+len, sizeof(header_buf)-len-1, "%s\r\n", m->response_status);
        for(int i=0; i<m->num_headers; i++) {
            if(strcmp("Transfer-Encoding", m->headers[i][0]) == 0) {
                // 修改chunked方式，
                len += snprintf(header_buf+len, sizeof(header_buf)-len-1, "_%s: %s\r\n", m->headers[i][0], m->headers[i][1]);
                continue;
            }
            len += snprintf(header_buf+len, sizeof(header_buf)-len-1, "%s: %s\r\n", m->headers[i][0], m->headers[i][1]);
        }
        //len += snprintf(header_buf+len, sizeof(header_buf)-len-1, "\r\n");
    
        *headers = std::string(header_buf, len);
    }

    if(body) {
        *body = std::string(m->body, m->body_size);
    }

    return ;
}


static void _message_to_headers_body(struct message* m, std::map<std::string, std::string>* headers=NULL, std::string* body=NULL) {

    if(!m) return ;

    if(headers) {
        char header_buf[MAX_HTTP_HEADER_SIZE];
    
        int len = 0;
        headers->insert(std::pair<std::string, std::string>("_STATUS_", m->response_status));
        for(int i=0; i<m->num_headers; i++) {
            headers->insert(std::pair<std::string, std::string>(m->headers[i][0], m->headers[i][1]));
        }
    }

    if(body) {
        *body = std::string(m->body, m->body_size);
    }

    return ;
}


int HttpParser::parse(std::string& html_raw_data, std::string* headers, std::string* body, size_t* parsed_len) {
    
    reset();

    http_message_->b_parser_header = true;
    if(!headers)
        http_message_->b_parser_header = false;

    http_message_->b_parser_body = true;
    if(!body)
        http_message_->b_parser_body = false;

    size_t parsed_len_ = http_parser_execute(http_parser_, &settings_count_body, html_raw_data.c_str(), html_raw_data.size());
    if(parsed_len) *parsed_len = parsed_len_;

    _message_to_headers_body(http_message_, headers, body);

    return 0;
}

int HttpParser::parse(std::string& html_raw_data, std::map<std::string, std::string>* headers, std::string* body, size_t* parsed_len) {

    reset();
 
    http_message_->b_parser_header = true;
    if(!headers)
        http_message_->b_parser_header = false;

    http_message_->b_parser_body = true;
    if(!body)
        http_message_->b_parser_body = false;
   
    size_t parsed_len_ = http_parser_execute(http_parser_, &settings_count_body, html_raw_data.c_str(), html_raw_data.size());
    if(parsed_len) *parsed_len = parsed_len_;
    
    _message_to_headers_body(http_message_, headers, body);

    return 0;
}
