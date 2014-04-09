
top_dir = ../..

AM_CPPFLAGS = -g \
           -I$(top_dir)/src/thrift \
           -I$(top_dir)/src/thrift/gen-cpp \
           -I$(top_dir)/common/cJSON \
           -I$(top_dir)/common/xpath/pugixml-1.4 \
           -I$(top_dir)/common/http_parser \
           -I/usr/local/glog-0.3.3/include -I${top_dir}/libs/glog-0.3.3/include \
           -I/usr/local/thrift-0.9.1/include -I${top_dir}/libs/thrift-0.9.1/include 
           

LIBS = -L. \
       -L/usr/local/glog-0.3.3/lib -L${top_dir}/libs/glog-0.3.3/lib -lglog \
       -L/usr/local/thrift-0.9.1/lib -I${top_dir}/libs/thrift-0.9.1/lib -lthrift

noinst_PROGRAMS = xpath_tools xpath_client
xpath_tools_SOURCES = xpath_tools.cpp
xpath_tools_LDADD = $(top_dir)/common/cJSON/libcJSON.a  \
                    $(top_dir)/common/xpath/pugixml-1.4/libpugixml.a \
                    $(top_dir)/common/http_parser/libHttpParser.a 

xpath_client_SOURCES = xpath_client.cpp
xpath_client_LDADD = $(top_dir)/common/cJSON/libcJSON.a  \
                     $(top_dir)/common/xpath/pugixml-1.4/libpugixml.a \
                     $(top_dir)/common/http_parser/libHttpParser.a  \
                     $(top_dir)/src/thrift/libspider_webservice.a


