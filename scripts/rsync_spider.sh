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

# 同步目标目录
host_file=$WORKDIR/conf/hosts.txt
DST_WORKDIR=/da1/first_engine/spider

echo -n "rsync $WORKDIR to $DST_WORKDIR at $host_file, are you sure(yes|no)? " 
read yesorno
if [ "$yesorno" != "yes" -o "$yesorno" != "y" ]; then
    echo "process cancelled"
    exit 1
fi



# 使用说明：./rsync-tool-single -h
$WORKDIR/sbin/rsync-tool-single -c $host_file -f $WORKDIR -d $DST_WORKDIR

