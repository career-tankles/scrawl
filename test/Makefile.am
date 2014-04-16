
top_dir = ..
AM_CPPFLAGS = -g -I${top_dir}/src \
                 -I${top_dir}/src/spider \
                 -I${top_dir}/common \
                 -I${top_dir}/common/uri \
                 -I${top_dir}/common/utils \
                 -I${top_dir}/libs/boost-1.50.0/include -I/usr/local/boost-1.50.0/include \
                 -I${top_dir}/libs/gflags-1.2/include -I/usr/local/gflags-1.2/include \
                 -I${top_dir}/libs/glog-0.3.3/include -I/usr/local/glog-0.3.3/include \
                 -I${top_dir}/libs/libevent-1.4.10/include -I/usr/local/libevent-1.4.10/include

LIBS = -L${top_dir}/libs/glog-0.3.3/lib -L/usr/local/glog-0.3.3/lib -lglog \
       -L${top_dir}/libs/gflags-1.2/lib -L/usr/local/gflags-1.2/lib -lgflags \
       -L${top_dir}/libs/boost-1.50.0/lib -L/usr/local/boost-1.50.0/lib -lboost_thread -lboost_system \
       -L${top_dir}/libs/libevent-1.4.10/lib -L/usr/local/libevent-1.4.10/lib -levent 

noinst_PROGRAMS = uri_tools download_tools
uri_tools_SOURCES = uri_tools.cc
uri_tools_LDADD = ${top_dir}/common/uri/liburi.a

download_tools_SOURCES = download-tools.cc
download_tools_LDADD = ${top_dir}/src/spider/libSpiderResManager.a




