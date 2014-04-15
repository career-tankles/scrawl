
AM_CPPFLAGS =   -I/usr/local/boost-1.50.0/include \
                -I/usr/local/glog-0.3.3/include \
                -I$(srcdir)/include \
                -I$(srcdir)/common  \
                -I$(srcdir)/common/utils \
                -I$(srcdir)/src 

LIBS =  -L/usr/local/boost-1.50.0/lib -lboost_thread  \
        -L/usr/local/glog-0.3.3/lib -lglog 

SUBDIRS = common src scripts test

dist_data_DATA = conf/spider.gflags conf/tpls.conf conf/tpl.m.baidu.com.conf

install-exec-hook:
	cd $(prefix) && rm -rf conf && $(LN_S) share conf; 

uninstall-hook:
	cd $(prefix) && rm -rf conf

