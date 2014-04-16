
top_dir = ../..

AM_CPPFLAGS =   -g -I. \
                -I${top_dir}/libs/thrift-0.9.1/include -I/usr/local/thrift-0.9.1/include \
                -I${top_dir}/libs/glog-0.3.3/include -I/usr/local/glog-0.3.3/include \
                -I${top_dir}/libs/gflags-1.2/include -I/usr/local/gflags-1.2/include \
                -I${top_dir}/src/thrift \
                -I${top_dir}/src/thrift/gen-cpp \
                -I${top_dir}/common/md5     \
                -I${top_dir}/common/cJSON   \
                -I${top_dir}/common/utils 

LIBS =  -L. \
        -L${top_dir}/libs/thrift-0.9.1/lib -L/usr/local/thrift-0.9.1/lib -lthrift \
        -L${top_dir}/libs/glog-0.3.3/lib -L/usr/local/glog-0.3.3/lib -lglog \
        -L${top_dir}/libs/gflags-1.2/lib -L/usr/local/gflags-1.2/lib  -lgflags

bin_PROGRAMS = multi_client 
multi_client_SOURCES = multi_client.cpp 
multi_client_LDADD   = ${top_dir}/common/md5/libmd5.a \
                     ${top_dir}/src/thrift/libspider_webservice.a

bin_PROGRAMS += dispatcher
dispatcher_SOURCES = dispatcher.cpp 
dispatcher_LDADD   = ${top_dir}/common/md5/libmd5.a \
                     ${top_dir}/src/thrift/libspider_webservice.a

