
top_dir = ../..

AM_CPPFLAGS =   -I/usr/local/thrift-0.9.1/include \
                -I../thrift/gen-cpp

LIBS =  -L/usr/local/thrift-0.9.1/lib -lthrift

noinst_PROGRAMS = client
client_SOURCES = client.cpp
client_LDADD   = ../thrift/libspider_webservice.a

