
AM_CPPFLAGS = -g -I../src -I../src/uri -I../common/ -I../common/utils \
                 -I/usr/local/boost-1.50.0/include \
                 -I/usr/local/gflags-1.2/include \
                 -I/usr/local/glog-0.3.3/include \
                 -I/usr/local/libevent-1.4.10/include 

LIBS = -L/usr/local/glog-0.3.3/lib -lglog \
       -L/usr/local/gflags-1.2/lib -lgflags \
       -L/usr/local/boost-1.50.0/lib -lboost_thread \
       -L/usr/local/libevent-1.4.10/lib -levent

noinst_PROGRAMS = uri_tools download_tools
uri_tools_SOURCES = uri_tools.cc
uri_tools_LDADD = ../src/uri/liburi.a

download_tools_SOURCES = download-tools.cc
download_tools_LDADD = ../src/libSpiderResManager.a




