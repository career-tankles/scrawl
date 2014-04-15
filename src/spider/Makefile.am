

CXXFLAGS = -g

top_dir = ../..
AM_CPPFLAGS = -g \
                -I/usr/local/boost-1.50.0/include -I${top_dir}/libs/boost-1.50.0/include \
                -I/usr/local/glog-0.3.3/include -I${top_dir}/libs/glog-0.3.3/include \
                -I/usr/local/gflags-1.2/include -I${top_dir}/libs/gflags-1.2/include \
                -I/usr/local/thrift-0.9.1/include -I${top_dir}/libs/thrift-0.9.1/include \
                -I/usr/local/libevent/include -I${top_dir}/libs/libevent/include -I${top_dir}/libs/libevent-1.4.10/include \
                -I${top_dir}/include \
                -I${top_dir}/common \
                -I${top_dir}/common/utils \
                -I${top_dir}/common/cJSON \
                -I${top_dir}/common/http_parser \
                -I${top_dir}/common/uri \
                -I${top_dir}/src/thrift  \
                -I${top_dir}/src/thrift/gen-cpp

LIBS =  -L. \
        -L/usr/local/boost-1.50.0/lib -L${top_dir}/libs/boost-1.50.0/lib -lboost_thread -lboost_system \
        -L/usr/local/glog-0.3.3/lib -L${top_dir}/libs/glog-0.3.3/lib -lglog     \
        -L/usr/local/gflags-1.2/lib -L${top_dir}/libs/gflags-1.2/lib -lgflags   \
        -L/usr/local/thrift-0.9.1/lib -L${top_dir}/libs/thrift-0.9.1/lib -lthrift \
        -L/usr/local/libevent/lib -L${top_dir}/libs/libevent/lib -L${top_dir}/libs/libevent-1.4.10/lib -levent 
        

noinst_LIBRARIES = libSpiderResManager.a
libSpiderResManager_a_SOURCES = SpiderResManager.cc message.cc dns.cc download.cc storage.cc TimeWait.cc message.cc conf.cc

#noinst_PROGRAMS = service_spider 
bin_PROGRAMS = service_spider 
service_spider_SOURCES = main.cc
service_spider_LDADD   = libSpiderResManager.a \
                         ${top_dir}/common/uri/liburi.a \
                         ${top_dir}/src/thrift/libspider_webservice.a \
                         ${top_dir}/common/cJSON/libcJSON.a \
                         ${top_dir}/common/http_parser/libHttpParser.a

SPIDER_SYMBOL_FILES = spider
install-exec-hook:
	cd $(bindir) && \
    for i in $(SPIDER_SYMBOL_FILES); \
    do \
        rm -f $$i && \
        $(LN_S) service_spider $$i; \
    done

uninstall-hook:
	cd $(bindir) && \
    for i in $(SPIDER_SYMBOL_FILES); \
    do \
        rm -f $$i; \
    done

