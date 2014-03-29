
top_dir = ../..
AM_CPPFLAGS =   -I/usr/local/boost-1.50.0/include 

LIBS =   \
        -L./thrift \
        -L/usr/local/boost-1.50.0/lib -lboost_thread -lboost_system \
        -L/usr/local/glog-0.3.3/lib -lglog     \
        -L/usr/local/gflags-1.2/lib -lgflags   \
        -L/usr/local/thrift-0.9.1/lib -lthrift \
        -levent
        

noinst_LIBRARIES = liburi.a
liburi_a_SOURCES = authority.cpp fragment.cpp ipv6_address.cpp  query.cpp   uri.cpp  urisyn.cpp domain_name.cpp ip_address.cpp  path.cpp scheme.cpp URI.cpp utils.cpp url-parser.c


