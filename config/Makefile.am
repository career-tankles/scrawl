
dist_data_DATA = spider.gflags dispatcher.gflags page_extractor.gflags search_list_client.gflags query_client.gflags \
                 hosts.txt spider.host.txt \
                 tpls.conf tpls.baidu.searchlist.conf \
                 tpl.3g.163.com.conf tpl.3g.d.cn.conf tpl.3g.gfan.com.conf tpl.7723.com.conf tpl.a.4399.cn.conf tpl.a.9game.cn.conf tpl.a.d.cn.conf tpl.a.gamedog.cn.conf tpl.app.zol.com.cn.conf tpl.a.yruan.com.conf tpl.d.91.com.conf tpl.g.10086.cn.conf tpl.game.3g.cn.conf tpl.g.pconline.com.cn.conf tpl.liqucn.com.conf tpl.m.159.com.conf tpl.m.18183.com.conf tpl.m.3533.com.conf tpl.m.520apk.com.conf tpl.m.91.com.conf tpl.m.angeeks.com.conf tpl.m.anruan.com.conf tpl.m.anzhi.com.conf tpl.m.anzhuo.com.conf tpl.m.apk8.com.conf tpl.m.apkzu.com.conf tpl.m.apkzu.com.conf.bak tpl.m.appchina.com.conf tpl.m.appfun.cn.conf tpl.m.baidu.com.conf tpl.m.cnmo.com.conf tpl.m.ggg.cn.conf tpl.m.hiapk.com.conf tpl.m.mumayi.com.conf tpl.m.muzhiwan.com.conf tpl.m.nduoa.com.conf tpl.m.pc6.com.conf tpl.m.sjxyx.com.conf tpl.m.yingyong.so.conf tpl.sjapk.com.conf tpl.soft.3g.cn.conf tpl.soxan.cn.conf tpl.wap.anzow.com.conf tpl.wap.crossmo.com.conf tpl.wap.shouji.com.cn.conf tpl.www.7k7k.com.conf tpl.www.paojiao.com.conf tpl.www.wandoujia.com.conf

install-exec-hook:
	cd $(prefix) && rm -rf conf && $(LN_S) share conf; 

uninstall-hook:
	cd $(prefix) && rm -rf conf

