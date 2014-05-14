
top_dir = ../..

AM_CPPFLAGS = -g \
           -I$(top_dir)/common \
           -I${top_dir}/common/md5 \
           -I$(top_dir)/common/utils \
           -I$(top_dir)/common/nlp/include 
 
LIBS = -L. \
       -L$(top_dir)/common/nlp/lib -lnlpanalysis -lsynonym -lsegwords -lutility -ldictEncode -lsentencestructure -lQueryClassify 

bin_PROGRAMS = query_filter
query_filter_SOURCES = query_filter.cpp
query_filter_LDADD = 

