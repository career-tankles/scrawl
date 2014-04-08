top_dir = ../..
CXXFLAGS = -g \
           -I$(top_dir)/common/cJSON \
           -I$(top_dir)/common/xpath/pugixml-1.4

noinst_PROGRAMS = xpath_tools
xpath_tools_SOURCES = xpath_tools.cpp
xpath_tools_LDADD = $(top_dir)/common/xpath/pugixml-1.4/libpugixml.a \
                    $(top_dir)/common/cJSON/libcJSON.a

