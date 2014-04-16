#!/bin/sh

ulimit -c unlimited


SYSTEM=$(uname -s)
RUNUSER=$(id -un)
CPUNUM=$(cat /proc/cpuinfo|grep processor|wc -l)

SCRIPT_PWD="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
export WORKDIR=`dirname $SCRIPT_PWD`

if [ -z "$WORKDIR" ]; then
    echo "WORKDIR is empty, set first!!"
    exit 1
fi
echo "WORKDIR=$WORKDIR"

#WORKDIR='/home/wangfengliang/scrawl/trunk'
#WORKDIR='/home/wangfengliang/mse/first_engine/scrawl/trunk'
BIN_DIR="$WORKDIR/bin"
PROC="service_spider"
SPIDER_BIN="$BIN_DIR/$PROC"
SPIDER_ARGS=" --flagfile=$WORKDIR/conf/spider.gflags "

SPIDER_BIN="$WORKDIR/bin/service_spider "
EXTRACTOR_BIN="$WORKDIR/bin/xpath_tools "
QUERY_CLIENT_BIN="$WORKDIR/bin/query_client "
SEARCH_LIST_CLIENT_BIN="$WORKDIR/bin/search_list_client "

KILL='killall '
KILL_FORCE='killall -9 '


export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$WORKDIR/libs/thrift-0.9.1/lib:$WORKDIR/libs/boost-1.50.0/lib:$WORKDIR/libs/gflags-1.2/lib:$WORKDIR/libs/glog-0.3.3/lib:$WORKDIR/libs/libevent-1.4.10/lib:$WORKDIR/libs/libcrypto:$WORKDIR/libs/libssl

function _log() {
    local logfile=$1
    local msg=$2
    local datetime=$(date "+%Y-%m-%d %H:%M:%S")
    echo "[ $datetime ] $msg" >> $logfile 2>&1
    [ $? -ne 0 ] && echo "[ $logfile ] write failed"
}

function _procinfo()
{
    local proc="$1"
    local wrkpath="$2"
    pgrep -u $RUNUSER -lf $proc |grep "$wrkpath" | fgrep -v grep 
}

#*************************************************
function _procnum() {
    local proc="$1"
    local wrkpath="$2"
    local run=0
    run=$(pgrep -u $RUNUSER -lf $proc|grep "$wrkpath"| fgrep -v grep | wc -l)
    [ $run -gt 0 ] &&  echo "$run" || echo '0'
}


#/********************************************************/
function _dostopproc() {
    local proc="$1"
    local kill_times=0
    local proc_num=$(_procnum "$proc" "$BIN_DIR")
    if [ $proc_num -eq 0 ];then
        echo "$proc maybe not running"
        return 0
    fi
    while [ $proc_num -ne 0 -a $kill_times -le 10 ];do
        $KILL $BIN_DIR/$proc >/dev/null 2>&1
        sleep 1
        #proc_num=$(pgrep -U $RUNUSER -x $proc|wc -l)
        proc_num=$(_procnum "$proc" "$BIN_DIR")
        kill_times=$(($kill_times+1))
    done
        #proc_num=$(pgrep -U $RUNUSER -x $proc|wc -l)
    if [ $proc_num -ne 0 ];then
        echo "$proc stop failed..."
        return 1
    else
        echo "$proc stoped!"
        return 0
    fi
}
#/********************************************************/


#/********************************************************/
function _dokillproc() {
    local proc="$1"
    local kill_times=0
    local proc_num=$(_procnum "$proc" "$BIN_DIR")
    if [ $proc_num -eq 0 ];then
        echo "$proc maybe not running"
        return 0
    fi
    while [ $proc_num -ne 0 -a $kill_times -le 10 ];do
        $KILL_FORCE $BIN_DIR/$proc >/dev/null 2>&1
        sleep 1
        #proc_num=$(pgrep -U $RUNUSER -x $proc|wc -l)
        proc_num=$(_procnum "$proc" "$BIN_DIR")
        kill_times=$(($kill_times+1))
    done
        #proc_num=$(pgrep -U $RUNUSER -x $proc|wc -l)
    if [ $proc_num -ne 0 ];then
        echo "$proc stop failed..."
        return 1
    else
        echo "$proc stoped!"
        return 0
    fi
}
#/********************************************************/

function _start_spider_()
{
    msg="start $SPIDER_BIN $SPIDER_ARGS"
    echo $msg' ...'
    $SPIDER_BIN $SPIDER_ARGS >> $WORKDIR/log/spider.log 2>&1 &
    _log "$WORKDIR/log/start.log" "$msg"
}

function _stop_spider_()
{
    echo "stoping $PROC ..."
    _dostopproc $PROC
    if [ $? -ne 0 ];then
        _log "$WORKDIR/log/start.log" "stop failed!"
    else
        _log "$WORKDIR/log/start.log" "stop success!"
    fi
}

function _kill_spider_()
{
    echo "stoping $PROC ..."
    _dokillproc $PROC
    if [ $? -ne 0 ];then
        _log "$WORKDIR/log/start.log" "stop failed!"
    else
        _log "$WORKDIR/log/start.log" "stop success!"
    fi
}

function _info_spider_()
{
    echo "$PROC process:"
    _procinfo $PROC $BIN_DIR
}


function _query_client_()
{
    server_addr="$1"
    server_port="$2"
    query_file="$3"
    $QUERY_CLIENT_BIN --CLIENT_server_addr=$server_addr --CLIENT_server_port=$server_port --CLIENT_query_file=$query_file
    if [ "$?" != "0" ]; then
        echo "$data_file send data error!!!"
        return 1
    else
        echo "$data_file send data finished!!!"
        return 0
    fi

}

function _search_list_client_()
{
    server_addr="$1"
    server_port="$2"
    data_format="$3"
    data_file="$4"
    $SEARCH_LIST_CLIENT_BIN --CLIENT_server_addr=$server_addr --CLIENT_server_port=$server_port --CLIENT_input_format=$data_format --CLIENT_input_file=$data_file
    if [ "$?" != "0" ]; then
        echo "$data_file send data error!!!"
        return 1
    else
        echo "$data_file send data finished!!!"
        return 0
    fi
}

function _extract_data_() 
{
    local tpls_conf_file="$1"
    local data_list_file="$2"
    local out_json_file="$3"

    mkdir -p log/extractor
    mkdir -p data/extractor
    
    has_errors=0
    cat $data_list_file | while read line;
    do
        data_file=$line
    
        filename=`basename $data_file`
        log_file="log/extractor/${filename}.log"
        echo "$data_file extracting ..."
        $EXTRACTOR_BIN --EXTRACTOR_input_tpls_file=$tpls_conf_file --EXTRACTOR_input_format=JSON --EXTRACTOR_input_file=$data_file --EXTRACTOR_output_file=$out_json_file > $log_file 2>&1
        if [ "$?" != "0" ]; then
            echo "$data_file extract data error!!!"
            echo "$data_file" > ${data_list_file}.error
            let has_errors=$has_errors+1
        else
            echo "$data_file extract data finished!!!"
            echo "$data_file" > ${data_list_file}.suc
        fi
    done
    
    if [ "$has_errors" != "0" ]; then
        echo "Extract data finished with errors, ref ${data_list_file}.error"
        return 1
    else
        echo "Extract data finished!!!"
        return 0
    fi
}


CMDNAME=`basename "$0"`

if [ "$CMDNAME" = 'start_spider.sh' ];then
    _start_spider_ 
    sleep 2
    _info_spider_ 
fi

if [ "$CMDNAME" = 'stop_spider.sh' ];then
    _stop_spider_ 
fi

if [ "$CMDNAME" = 'kill_spider.sh' ];then
    _kill_spider_ 
fi

if [ "$CMDNAME" = 'restart_spider.sh' ];then
    _stop_spider_ 
    sleep 2
    _start_spider_ 
    sleep 2
    _info_spider_ 
fi

if [ "$CMDNAME" = 'info_spider.sh' ];then
    _info_spider_ 
fi

if [ "$CMDNAME" = "query_client.sh" ];then
    if [ "$#" != "3" ]; then
        echo "Usage: $CMDNAME <server_addr> <server_port> <query_file>"
        exit 1
    fi
    server_addr="$1"
    server_port="$2"
    query_file="$3"
    _query_client_ "$server_addr" $server_port "$query_file"
    
fi

#data_list_file='spider-data-file.lst'
if [ "$CMDNAME" = "extract_data.sh" ];then
    if [ "$#" != "3" ]; then
        echo "Usage: $CMDNAME <tpls_conf_file> <data_list_file> <out_json_file>"
        exit 1
    fi
    tpls_conf_file="$1"
    data_list_file="$2"
    out_json_file="$3"

    if [ ! -f "$tpls_conf_file" ]; then
        echo "$tpls_conf_file not exist or no regular file"
        exit 1
    fi

    if [ ! -f "$data_list_file" ]; then
        echo "$data_list_file not exist or no regular file"
        exit 1
    fi

    if [ -e "$out_json_file" ]; then
        echo "$out_json_file exist!"
        exit 1
    fi
    _extract_data_ "$tpls_conf_file" "$data_list_file" "$out_json_file"
    
fi

if [ "$CMDNAME" = "search_list_client.sh" ];then
    if [ "$#" != "3" ]; then
        echo "Usage: $CMDNAME <server_addr> <server_port> <search_list_json_file>"
        exit 1
    fi
    server_addr="$1"
    server_port=$2
    search_list_json_file="$3"
    data_format="JSON"
    _search_list_client_ "$server_addr" $server_port "$data_format" "$search_list_json_file"
    
fi

