
top_dir = ../..

AM_CPPFLAGS =   -I/usr/local/thrift-0.9.1/include \
                -I/usr/local/boost-1.50.0/include -I${top_dir}/libs/boost-1.50.0/include \
                -I./gen-cpp

LIBS =  -L/usr/local/thrift-0.9.1/lib -lthrift \
        -L/usr/local/boost-1.50.0/lib -L${top_dir}/libs/boost-1.50.0/lib -lboost_thread -lboost_system

THRIFT = /usr/local/thrift-0.9.1/bin/thrift
gen-cpp/spider_constants.cpp:
	${THRIFT} -r --gen cpp spider.thrift
	${THRIFT} -r --gen py spider.thrift

#CLEANFILES = gen-cpp/* gen-py/*

noinst_LIBRARIES = libspider_webservice.a
libspider_webservice_a_SOURCES = gen-cpp/spider_constants.cpp gen-cpp/spider_types.cpp gen-cpp/SpiderWebService.cpp ThriftClientWrapper.cpp 


