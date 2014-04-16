
.PHONY: CTRL_SCRIPTS_SYMBOL

dist_sbin_SCRIPTS = setenv.sh ctrl.sh run.sh rsync-tool-single remote_cmds rsync_spider.sh

CTRL_SCRIPTS_SYMBOL_FILES = spider_start.sh spider_stop.sh spider_kill.sh spider_restart.sh spider_info.sh \
                            dispatcher_start.sh dispatcher_stop.sh dispatcher_kill.sh dispatcher_restart.sh dispatcher_info.sh \
                            query_client.sh extract_data.sh search_list_client.sh 

install-exec-hook:
	cd $(sbindir) && \
    for i in $(CTRL_SCRIPTS_SYMBOL_FILES); \
    do \
        rm -f $$i && \
        $(LN_S) ctrl.sh $$i; \
    done 

uninstall-hook:
	cd $(sbindir) && \
    for i in $(CTRL_SCRIPTS_SYMBOL_FILES); \
    do \
        rm -f $$i; \
    done

