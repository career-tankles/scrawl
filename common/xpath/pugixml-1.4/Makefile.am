
top_dir = ../../..
CXXFLAGS = -g -I$(top_dir)/common/cJSON

noinst_LIBRARIES = libxpath.a
libxpath_a_SOURCES = pugixml.cpp

noinst_PROGRAMS = xpath_tools
xpath_tools_SOURCES = xpath_tools.cpp
xpath_tools_LDADD = libxpath.a $(top_dir)/common/cJSON/libcJSON.a

