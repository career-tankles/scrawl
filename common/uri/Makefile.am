
top_dir = ../..
AM_CPPFLAGS = -I ${top_dir}/libs/boost-1.50.0/include -I/usr/local/boost-1.50.0/include 

LIBS =   \
        -L${top_dir}/libs/boost-1.50.0/lib -L/usr/local/boost-1.50.0/lib -lboost_thread -lboost_system 

noinst_LIBRARIES = liburi.a
liburi_a_SOURCES = authority.cpp fragment.cpp ipv6_address.cpp  query.cpp   uri.cpp  urisyn.cpp domain_name.cpp ip_address.cpp  path.cpp scheme.cpp URI.cpp utils.cpp url-parser.c


