#ifndef CONN_H_
#define CONN_H_

#define CONN_SEND_BUF_SIZE  1024
#define CONN_RECV_BUF_SIZE  1024*1024

#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <strings.h>
#include <cstdlib>
#include <cstdio>
#include <cstddef>
#include <string>
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include <event.h>

enum CONN_STAT {
    CONN_STAT_INIT=0,
    CONN_STAT_CONNECTING,
    CONN_STAT_WRITING,
    CONN_STAT_READING,
    CONN_STAT_FINISH,
    CONN_STAT_ERROR,
    CONN_STAT_TIMEOUT,
	CONN_STAT_MAX
};

typedef struct connection{

    int sock_fd_ ;
    char sock_ip_[32] ;
    unsigned short sock_port_ ;
    sockaddr_in sock_addr_;

    char send_buf_[CONN_SEND_BUF_SIZE+1] ;
	char* send_buf_rptr_;
	size_t send_buf_left_;

	char recv_buf_[CONN_RECV_BUF_SIZE];
	char* recv_buf_wptr_;
	size_t recv_buf_left_;
	size_t recv_buf_len_;

    CONN_STAT conn_stat_ ;

	int err_times;
	int err_code_;
	int counts;

    //ev_io watcher;
    struct event fd_event;
    void* user_data ;
    void* user_data2 ;
    void* user_data3 ;

    long conn_time ;            // 连接建立时间
    long write_end_time ;       // 请求发送时间
    long recv_begin_time ;      // 开始读取数据时间
    long recv_end_time ;        // 读取数据完毕时间(or出错时间)

    void (*cb_result_handler_)(connection*) ;           // callback for process fetch result
    
} connection, conn ;


static int conn_reset(conn* c) {
    assert(c && "conn is null") ;
    c->sock_fd_ = -1 ;
    memset(c->sock_ip_, 0, sizeof(c->sock_ip_)) ;
    c->sock_port_ = 0 ;
    c->counts = 0 ;

    c->conn_stat_ = CONN_STAT_INIT ;

    c->err_code_ = 0 ;
    c->err_times = 0 ;

    c->cb_result_handler_ = NULL ;

    c->user_data = NULL ;

    memset(c->recv_buf_, 0, sizeof(c->recv_buf_)) ;
	c->recv_buf_wptr_ = c->recv_buf_;
	c->recv_buf_left_ = sizeof(c->recv_buf_)-1;
	c->recv_buf_len_= 0;

    c->conn_time = 0;
    c->write_end_time = 0;
    c->recv_begin_time = 0;
    c->recv_end_time = 0;

    return 0 ;
}

static void _setaddress(const char* ip,int port,struct sockaddr_in* addr){
    bzero(addr,sizeof(*addr));
    addr->sin_family=AF_INET;
    inet_pton(AF_INET,ip,&(addr->sin_addr));
    addr->sin_port=htons(port);
}

static void _setnonblock(int fd){
    fcntl(fd,F_SETFL,fcntl(fd,F_GETFL) | O_NONBLOCK);
}

static void _setreuseaddr(int fd){
    int ok=1;
    setsockopt(fd,SOL_SOCKET,SO_REUSEADDR,&ok,sizeof(ok));
}

static int conn_init(conn* c, const char* ip, unsigned short port=80) {
    assert(c && "conn is null") ;
    c->sock_fd_ = socket(AF_INET,SOCK_STREAM,IPPROTO_TCP) ;
    _setnonblock(c->sock_fd_) ;
    _setreuseaddr(c->sock_fd_) ;
    _setaddress(ip, port, &(c->sock_addr_)) ;

    return 0 ;    
}

static std::string _address_to_string(struct sockaddr_in* addr){
    char ip[128];
    inet_ntop(AF_INET,&(addr->sin_addr),ip,sizeof(ip));
    char port[32];
    snprintf(port,sizeof(port),"%d",ntohs(addr->sin_port));
    std::string r;
    r=r+"("+ip+":"+port+")";
    return r;
}

static int conn_connect(conn* c) {
    assert(c && "conn is null") ;
    return connect(c->sock_fd_, (struct sockaddr*)(&(c->sock_addr_)), sizeof(c->sock_addr_));
}

static conn* conn_new(const char* ip, unsigned short port=80) {
    conn* c = new conn ;
    conn_reset(c) ;
    conn_init(c, ip, port) ;
    return c ;
}
static conn* conn_new_client(const char* ip, unsigned short port=80) {
    return conn_new(ip, port) ;
}

static void conn_free(conn* c) {
    assert(c && "conn is null") ;
    assert(c->sock_fd_ == -1 && "c->sock_fd_ may be not closed.") ;
    delete c ;
    c = NULL ;
}

static int conn_close(conn* c) {
    assert(c && "conn is null") ;
    assert(c->sock_fd_ != -1 && "c->sock_fd_ maybe have been closed.") ;
    int ret = close(c->sock_fd_) ;
    c->sock_fd_ = -1 ;
    return ret ;
}


static conn* conn_new_server(const char* ip="0.0.0.0", unsigned short port=80, unsigned short backlog=10000){
    conn* c = new conn ;
    conn_reset(c) ;
    conn_init(c, ip, port) ;
    bind(c->sock_fd_, (struct sockaddr*)&(c->sock_addr_), sizeof(c->sock_addr_));
    listen(c->sock_fd_, backlog);

    assert(c) ;
    assert(c->sock_fd_) ;
    
    return c ;
}



#endif

