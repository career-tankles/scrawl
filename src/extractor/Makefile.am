
top_dir = ../..

AM_CPPFLAGS = -g \
           -I$(top_dir)/common \
           -I$(top_dir)/common/utils \
           -I$(top_dir)/common/cJSON \
           -I$(top_dir)/common/xpath/pugixml-1.4 \
           -I$(top_dir)/common/http_parser \
           -I$(top_dir)/src/thrift \
           -I$(top_dir)/src/thrift/gen-cpp \
           -I${top_dir}/libs/glog-0.3.3/include -I/usr/local/glog-0.3.3/include \
           -I${top_dir}/libs/gflags-1.2/include -I/usr/local/gflags-1.2/include \
           -I${top_dir}/libs/thrift-0.9.1/include -I/usr/local/thrift-0.9.1/include 
           

LIBS = -L. \
       -L${top_dir}/libs/glog-0.3.3/lib -L/usr/local/glog-0.3.3/lib -lglog \
       -L${top_dir}/libs/gflags-1.2/lib -L/usr/local/gflags-1.2/lib -lgflags   \
       -L${top_dir}/libs/thrift-0.9.1/lib -L/usr/local/thrift-0.9.1/lib -lthrift

#bin_PROGRAMS = xpath_tools 
#xpath_tools_SOURCES = xpath_tools.cpp
#xpath_tools_LDADD = $(top_dir)/common/cJSON/libcJSON.a  \
#                    $(top_dir)/common/xpath/pugixml-1.4/libpugixml.a \
#                    $(top_dir)/common/http_parser/libHttpParser.a 
#
bin_PROGRAMS = page_extractor
page_extractor_SOURCES = page_extractor.cpp
page_extractor_LDADD = $(top_dir)/common/cJSON/libcJSON.a  \
                    $(top_dir)/common/xpath/pugixml-1.4/libpugixml.a \
                    $(top_dir)/common/http_parser/libHttpParser.a 


noinst_PROGRAMS = cfg_tpl_t
cfg_tpl_t_SOURCES = cfg_tpl_t.cpp
cfg_tpl_t_LDADD = $(top_dir)/common/cJSON/libcJSON.a 


