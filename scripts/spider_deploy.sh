#!/bin/sh

if [ -x "setenv.sh" ]; then
. setenv.sh
fi

SCRIPT_PWD="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
export WORKDIR=`dirname $SCRIPT_PWD`
if [ -z "$WORKDIR" ]; then
    echo "WORKDIR is empty, set first!!"
    exit 1
fi
echo "WORKDIR=$WORKDIR"

echo -n "will be deploying spider, host_file:$host_file, are you sure(yes|no)? " 
read yesorno
if [ "$yesorno" != "yes" -a "$yesorno" != "y" ]; then
    echo "process cancelled"
    exit 1
fi

dir_libs="/home/wangfengliang/mse/first_engine/scrawl/libs"
host_file="$WORKDIR/conf/spider.host.txt"
spider_ini_cmd_file="tmp.spider.env.init.sh"
spider_ini_cmd_file2="tmp.spider.env.post.sh"
password=''


echo "#!/bin/sh" > $spider_ini_cmd_file
echo "sudo mkdir -p /da1/msearch/first_engine" >> $spider_ini_cmd_file
echo "sudo mkdir -p /da1/msearch/first_engine/libs" >> $spider_ini_cmd_file
echo "sudo mkdir -p /da1/msearch/first_engine/data/query" >> $spider_ini_cmd_file
echo "sudo mkdir -p /da1/msearch/first_engine/data/spider" >> $spider_ini_cmd_file
echo "sudo mkdir -p /da1/msearch/first_engine/data/extractor" >> $spider_ini_cmd_file
echo "sudo mkdir -p /da1/msearch/first_engine/spider" >> $spider_ini_cmd_file
echo "sudo mkdir -p /da1/msearch/first_engine/spider/bin" >> $spider_ini_cmd_file
echo "sudo mkdir -p /da1/msearch/first_engine/spider/sbin" >> $spider_ini_cmd_file
echo "sudo mkdir -p /da1/msearch/first_engine/spider/share" >> $spider_ini_cmd_file
echo "sudo mkdir -p /da1/msearch/first_engine/spider/log" >> $spider_ini_cmd_file
echo "sudo ln -s /da1/msearch/first_engine/libs  /da1/msearch/first_engine/spider/libs" >> $spider_ini_cmd_file
echo "sudo ln -s /da1/msearch/first_engine/data  /da1/msearch/first_engine/spider/data" >> $spider_ini_cmd_file
echo "sudo ln -s /da1/msearch/first_engine/spider/share  /da1/msearch/first_engine/spider/conf" >> $spider_ini_cmd_file
echo "sudo chown -R wangfengliang.wangfengliang /da1/msearch/first_engine" >> $spider_ini_cmd_file

read -s -p 'passwd:' password

# 创建目录结构
$WORKDIR/sbin/super_remote_cmds.py --host_file=$host_file --cmd_file=$spider_ini_cmd_file --passwd_nosecure=$password

# 同步libs
#$WORKDIR/sbin/super_remote_cmds.py --host_file=$host_file --rsync_src_dir_or_file=$dir_libs --rsync_dst_dir=/da1/msearch/first_engine/ --passwd_nosecure=$password 
$WORKDIR/sbin/rsync-tool-single --host_file=$host_file --file=$dir_libs --dir=/da1/msearch/first_engine/

# 同步spider程序
#$WORKDIR/sbin/super_remote_cmds.py --host_file=$host_file --rsync_src_dir_or_file=$WORKDIR/ --rsync_dst_dir=/da1/msearch/first_engine/spider/ --passwd_nosecure=$password 
$WORKDIR/sbin/rsync-tool-single --host_file=$host_file --file=$WORKDIR --dir=/da1/msearch/first_engine/

# 修改owner
echo "#!/bin/sh" > $spider_ini_cmd_file2
echo "sudo chown -R msearch.msearch /da1/msearch/first_engine" >> $spider_ini_cmd_file2
$WORKDIR/sbin/super_remote_cmds.py --host_file=$host_file --cmd_file=$spider_ini_cmd_file2 --passwd_nosecure=$password 

