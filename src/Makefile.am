
SUBDIRS = thrift uri

CXXFLAGS = -g

top_dir = ..
AM_CPPFLAGS =   -I/usr/local/boost-1.50.0/include \
                -I/usr/local/glog-0.3.3/include \
                -I/usr/local/gflags-1.2/include \
                -I/usr/local/thrift-0.9.1/include \
                -I${top_dir}/include \
                -I${top_dir}/common \
                -I${top_dir}/common/uri \
                -I${top_dir}/common/utils \
                -I./uri \
                -I./thrift  \
                -I./thrift/gen-cpp

LIBS =  -L. \
        -L/usr/local/boost-1.50.0/lib -lboost_thread -lboost_system \
        -L/usr/local/glog-0.3.3/lib -lglog     \
        -L/usr/local/gflags-1.2/lib -lgflags   \
        -L/usr/local/thrift-0.9.1/lib -lthrift \
        -levent \
        -L./thrift \
        -L./uri 
        

noinst_LIBRARIES = libSpiderResManager.a
libSpiderResManager_a_SOURCES = SpiderResManager.cc message.cc dns.cc download.cc storage.cc TimeWait.cc message.cc

noinst_PROGRAMS    = thrift_spider 
thrift_spider_SOURCES = service-main.cc
thrift_spider_LDADD   = libSpiderResManager.a thrift/libspider_webservice.a uri/liburi.a

#noinst_PROGRAMS   += dns_tools download_tools TimeWait_tools
#dns_tools_SOURCES = dns-tools.cc
#dns_tools_LDADD   = libdns.a
#download_tools_SOURCES = download-tools.cc
#download_tools_LDADD = libdownload.a
#TimeWait_tools_SOURCES = TimeWait-tools.cc
#TimeWait_tools_LDADD = libtimewait.a libdownload.a 

