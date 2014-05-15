
.PHONY: CTRL_SCRIPTS_SYMBOL

dist_sbin_SCRIPTS = setenv.sh ctrl.sh run.sh rsync-tool-single remote_cmds rsync_spider.sh auto_ssh.sh auto_ssh_msearch.sh super_remote_cmds.py spider_deploy.sh spider_service.sh spider_service_gen_fetch_urls.py 

CTRL_SCRIPTS_SYMBOL_FILES = spider_start.sh spider_stop.sh spider_kill.sh spider_restart.sh spider_info.sh \
                            dispatcher_start.sh dispatcher_stop.sh dispatcher_kill.sh dispatcher_restart.sh dispatcher_info.sh \
                            query_client.sh extract_data.sh search_list_client.sh make_data_list.sh

SPIDER_SERVICE_SYMBOL_FILES = spider_service_extract_data.sh spider_service_extract_baidu_search_list.sh spider_service_parse_baidu_search_list.sh spider_service_gen_query_fetch_urls.sh spider_service_gen_search_list_fetch_urls.sh

install-exec-hook:
	cd $(sbindir) && \
    for i in $(CTRL_SCRIPTS_SYMBOL_FILES); \
    do \
        rm -f $$i && \
        $(LN_S) ctrl.sh $$i; \
    done 
	cd $(sbindir) && \
    for i in $(SPIDER_SERVICE_SYMBOL_FILES); \
    do \
        rm -f $$i && \
        $(LN_S) spider_service.sh $$i; \
    done 
    

uninstall-hook:
	cd $(sbindir) && \
    for i in $(CTRL_SCRIPTS_SYMBOL_FILES); \
    do \
        rm -f $$i; \
    done 
	cd $(sbindir) && \
    for i in $(SPIDER_SERVICE_SYMBOL_FILES); \
    do \
        rm -f $$i; \
    done 

