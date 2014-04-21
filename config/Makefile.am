
dist_data_DATA = spider.gflags dispatcher.gflags page_extractor.gflags search_list_client.gflags query_client.gflags \
                 tpls.conf tpl.m.baidu.com.conf tpl.www.wandoujia.com.conf \
                 hosts.txt spider.host.txt

install-exec-hook:
	cd $(prefix) && rm -rf conf && $(LN_S) share conf; 

uninstall-hook:
	cd $(prefix) && rm -rf conf

