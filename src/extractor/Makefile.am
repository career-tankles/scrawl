
top_dir = ../..

AM_CPPFLAGS = -g \
           -I$(top_dir)/common/cJSON \
           -I$(top_dir)/common/xpath/pugixml-1.4 \
           -I$(top_dir)/common/http_parser \
           -I/usr/local/glog-0.3.3/include -I${top_dir}/libs/glog-0.3.3/include 

LIBS = -L. \
       -L/usr/local/glog-0.3.3/lib -L${top_dir}/libs/glog-0.3.3/lib -lglog 

noinst_PROGRAMS = xpath_tools
xpath_tools_SOURCES = xpath_tools.cpp
xpath_tools_LDADD = $(top_dir)/common/cJSON/libcJSON.a  \
                    $(top_dir)/common/xpath/pugixml-1.4/libpugixml.a \
                    $(top_dir)/common/http_parser/libHttpParser.a 

