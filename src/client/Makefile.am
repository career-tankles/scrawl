
top_dir = ../..

AM_CPPFLAGS =   -g -I. \
                -I${top_dir}/libs/thrift-0.9.1/include -I/usr/local/thrift-0.9.1/include \
                -I${top_dir}/libs/glog-0.3.3/include -I/usr/local/glog-0.3.3/include \
                -I${top_dir}/libs/gflags-1.2/include -I/usr/local/gflags-1.2/include \
                -I${top_dir}/libs/boost-1.50.0/include -I/usr/local/boost-1.50.0/include \
                -I${top_dir}/src/thrift \
                -I${top_dir}/src/thrift/gen-cpp \
                -I${top_dir}/common/md5     \
                -I${top_dir}/common/cJSON   \
                -I${top_dir}/common/utils 

LIBS =  -L. \
        -L${top_dir}/libs/thrift-0.9.1/lib -L/usr/local/thrift-0.9.1/lib -lthrift \
        -L${top_dir}/libs/glog-0.3.3/lib -L/usr/local/glog-0.3.3/lib -lglog \
        -L${top_dir}/libs/boost-1.50.0/lib -L/usr/local/boost-1.50.0/lib \
        -L${top_dir}/libs/gflags-1.2/lib -L/usr/local/gflags-1.2/lib -lgflags

bin_PROGRAMS = query_client
query_client_SOURCES = query_client.cpp ThriftClientWrapper.cpp
query_client_LDADD   = ${top_dir}/common/md5/libmd5.a \
                       ${top_dir}/src/thrift/libspider_webservice.a

bin_PROGRAMS += search_list_client
search_list_client_SOURCES = search_list_client.cpp ThriftClientWrapper.cpp
search_list_client_LDADD = $(top_dir)/common/cJSON/libcJSON.a \
                           $(top_dir)/common/xpath/pugixml-1.4/libpugixml.a \
                           $(top_dir)/common/http_parser/libHttpParser.a  \
                           $(top_dir)/src/thrift/libspider_webservice.a

bin_PROGRAMS += dispatcher
dispatcher_SOURCES = dispatcher.cpp ThriftClientWrapper.cpp
dispatcher_LDADD   = ${top_dir}/common/md5/libmd5.a \
                     ${top_dir}/src/thrift/libspider_webservice.a

