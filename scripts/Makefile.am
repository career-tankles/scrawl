
.PHONY: CTRL_SCRIPTS_SYMBOL

dist_sbin_SCRIPTS = ctrl.sh run.sh rsync-tool-single remote_cmds


CTRL_SCRIPTS_SYMBOL_FILES = start_spider.sh stop_spider.sh kill_spider.sh restart_spider.sh info_spider.sh query_client.sh extract_data.sh search_list_client.sh 

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

