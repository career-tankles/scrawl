
SUBDIRS = word2vec 

top_dir = ../..

AM_CPPFLAGS = -g \
           -I$(top_dir)/common \
           -I${top_dir}/common/md5 \
           -I$(top_dir)/common/utils \
           -I$(top_dir)/common/cJSON \
           -I$(top_dir)/common/xpath/pugixml-1.4 \
           -I$(top_dir)/common/http_parser \
           -I$(top_dir)/common/febird/febird/include \
           -I$(top_dir)/common/nlp/include \
           -I$(top_dir)/common/php_serialize \
           -I$(top_dir)/src/thrift \
           -I$(top_dir)/src/thrift/gen-cpp \
           -I$(top_dir)/src/data_merge/word2vec \
           -I$(top_dir)/src/data_merge/php_serialize \
           -I${top_dir}/libs/glog-0.3.3/include -I/usr/local/glog-0.3.3/include \
           -I${top_dir}/libs/gflags-1.2/include -I/usr/local/gflags-1.2/include \
           -I${top_dir}/libs/boost-1.50.0/include -I/usr/local/boost-1.50.0/include 
 
LIBS = -L. \
       -lcurl \
       -L$(top_dir)/common/cJSON -lcJSON  \
       -L${top_dir}/common/md5 -lmd5 \
       -L$(top_dir)/common/nlp/lib -lnlpanalysis -lsynonym -lsegwords -lutility -ldictEncode -lsentencestructure -lQueryClassify \
	   -L$(top_dir)/common/php_serialize -lutil_php_serialize \
       -L${top_dir}/libs/glog-0.3.3/lib -L/usr/local/glog-0.3.3/lib -lglog \
       -L${top_dir}/libs/gflags-1.2/lib -L/usr/local/gflags-1.2/lib -lgflags \
       -L${top_dir}/libs/boost-1.50.0/lib -L/usr/local/boost-1.50.0/lib -lgflags 

bin_PROGRAMS = data_merge
data_merge_SOURCES = main.cpp httpclient.cpp similarity_calculation.cpp
data_merge_LDADD = 

