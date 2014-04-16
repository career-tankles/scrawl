

SUBDIRS = common src scripts test

dist_data_DATA = config/spider.gflags config/dispatcher.gflags \
                 config/tpls.conf config/tpl.m.baidu.com.conf \
                 config/hosts.txt

install-exec-hook:
	cd $(prefix) && rm -rf conf && $(LN_S) share conf; 
	cd $(prefix) && mkdir -p log 

uninstall-hook:
	cd $(prefix) && rm -rf conf

