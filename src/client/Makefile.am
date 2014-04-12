
top_dir = ../..

AM_CPPFLAGS =   -I. \
                -I/usr/local/thrift-0.9.1/include -I${top_dir}/libs/thrift-0.9.1/include \
                -I/usr/local/glog-0.3.3/include -I${top_dir}/libs/glog-0.3.3/include \
                -I/usr/local/gflags-1.2/include -I${top_dir}/libs/gflags-1.2/include \
                -I${top_dir}/src/thrift/gen-cpp \
                -I${top_dir}/common/md5

LIBS =  -L. \
        -L/usr/local/thrift-0.9.1/lib -I${top_dir}/libs/thrift-0.9.1/lib -lthrift \
        -L/usr/local/glog-0.3.3/lib -L${top_dir}/libs/glog-0.3.3/lib -lglog \
        -L/usr/local/gflags-1.2/lib -L${top_dir}/libs/gflags-1.2/lib -lgflags

noinst_PROGRAMS = client
client_SOURCES = client.cpp
client_LDADD   = ${top_dir}/common/md5/libmd5.a \
                 ${top_dir}/src/thrift/libspider_webservice.a

