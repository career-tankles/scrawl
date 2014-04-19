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

BIN_DIR="$WORKDIR/bin"

SPIDER_PROC="service_spider"
SPIDER_BIN="$WORKDIR/bin/service_spider "
SPIDER_ARGS=" --flagfile=$WORKDIR/conf/spider.gflags "

DISPATCHER_PROC="dispatcher"
DISPATCHER_BIN="$WORKDIR/bin/dispatcher "
DISPATCHER_ARGS=" --flagfile=$WORKDIR/conf/dispatcher.gflags "

PAGE_EXTRACTOR_PROC="page_extractor"
PAGE_EXTRACTOR_BIN="$WORKDIR/bin/page_extractor "
PAGE_EXTRACTOR_ARGS=" --flagfile=$WORKDIR/conf/page_extractor.gflags "

SEARCH_LIST_CLIENT_PROC="search_list_client"
SEARCH_LIST_CLIENT_BIN="$WORKDIR/bin/search_list_client "
SEARCH_LIST_CLIENT_ARGS=" --flagfile=$WORKDIR/conf/search_list_client.gflags "

QUERY_CLIENT_PROC="query_client"
QUERY_CLIENT_BIN="$WORKDIR/bin/query_client "
QUERY_CLIENT_ARGS=" --flagfile=$WORKDIR/conf/query_client.gflags "

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

function _start_dispatcher_()
{
    msg="start $DISPATCHER_BIN $DISPATCHER_ARGS"
    echo $msg' ...'
    $DISPATCHER_BIN $DISPATCHER_ARGS > $WORKDIR/log/dispatcher.log 2>&1 &
    _log "$WORKDIR/log/start.log" "$msg"
}

function _stop_dispatcher_()
{
    echo "stoping $DISPATCHER_PROC ..."
    _dostopproc $DISPATCHER_PROC
    if [ $? -ne 0 ];then
        _log "$WORKDIR/log/start.log" "stop failed!"
    else
        _log "$WORKDIR/log/start.log" "stop success!"
    fi
}

function _kill_dispatcher_()
{
    echo "stoping $DISPATCHER_PROC ..."
    _dokillproc $DISPATCHER_PROC
    if [ $? -ne 0 ];then
        _log "$WORKDIR/log/start.log" "stop failed!"
    else
        _log "$WORKDIR/log/start.log" "stop success!"
    fi
}

function _info_dispatcher_()
{
    echo "$DISPATCHER_PROC process:"
    _procinfo $DISPATCHER_PROC $BIN_DIR
}

function _start_spider_()
{
    msg="start $SPIDER_BIN $SPIDER_ARGS"
    echo $msg' ...'
    $SPIDER_BIN $SPIDER_ARGS > $WORKDIR/log/spider.log 2>&1 &
    _log "$WORKDIR/log/start.log" "$msg"
}

function _stop_spider_()
{
    echo "stoping $SPIDER_PROC ..."
    _dostopproc $SPIDER_PROC
    if [ $? -ne 0 ];then
        _log "$WORKDIR/log/start.log" "stop failed!"
    else
        _log "$WORKDIR/log/start.log" "stop success!"
    fi
}

function _kill_spider_()
{
    echo "stoping $SPIDER_PROC ..."
    _dokillproc $SPIDER_PROC
    if [ $? -ne 0 ];then
        _log "$WORKDIR/log/start.log" "stop failed!"
    else
        _log "$WORKDIR/log/start.log" "stop success!"
    fi
}

function _info_spider_()
{
    echo "$SPIDER_PROC process:"
    _procinfo $SPIDER_PROC $BIN_DIR
}


function _query_client_()
{
    log_file="log/query_client.log"
    if [ "$#" == "0" ]; then
        $QUERY_CLIENT_BIN $QUERY_CLIENT_ARGS > $log_file 2>&1
        if [ "$?" != "0" ]; then
            echo "query_client failed, gflags=$QUERY_CLIENT_ARGS"
            return 1
        else
            echo "query_client finished, gflags=$QUERY_CLIENT_ARGS"
            return 0
        fi
    elif [ "$#" == "1" ]; then
        gflags_file="$1"
        $QUERY_CLIENT_BIN --flagfile=$gflags_file > $log_file 2>&1
        if [ "$?" != "0" ]; then
            echo "query_client failed, gflags=$gflags_file"
            return 1
        else
            echo "query_client finished, gflags=$gflags_file"
            return 0
        fi
    elif [ "$#" == "2" ]; then
        server_addr="$1"
        query_file="$2"
        $QUERY_CLIENT_BIN --CLIENT_server_addrs=$server_addr --CLIENT_query_file=$query_file
        if [ "$?" != "0" ]; then
            echo "$data_file send data error!!!"
            return 1
        else
            echo "$data_file send data finished!!!"
            return 0
        fi
    fi
}

function _search_list_client_()
{
    log_file="log/search_list_client.log"
    if [ "$#" == "0" ]; then
        $SEARCH_LIST_CLIENT_BIN $SEARCH_LIST_CLIENT_ARGS > $log_file 2>&1
        if [ "$?" != "0" ]; then
            echo "search_list_client failed, gflags=$SEARCH_LIST_CLIENT_ARGS"
            return 1
        else
            echo "search_list_client finished, gflags=$SEARCH_LIST_CLIENT_ARGS"
            return 0
        fi
    elif [ "$#" == "1" ]; then
        gflags_file="$1"
        $SEARCH_LIST_CLIENT_BIN --flagfile=$gflags_file > $log_file 2>&1
        if [ "$?" != "0" ]; then
            echo "search_list_client failed, gflags=$gflags_file"
            return 1
        else
            echo "search_list_client failed, gflags=$gflags_file"
            return 0
        fi
    elif [ "$#" == "3" ]; then
        server_addr="$1"
        data_format="$2"
        data_file="$3"
        $SEARCH_LIST_CLIENT_BIN --CLIENT_server_addrs=$server_addr --CLIENT_input_format=$data_format --CLIENT_input_file=$data_file > $log_file 2>&1
        if [ "$?" != "0" ]; then
            echo "$data_file send data error!!!"
            return 1
        else
            echo "$data_file send data finished!!!"
            return 0
        fi
    fi
}

function _extract_data_() 
{
    log_file="log/page_extractor.log"
    if [ "$#" == "0" ]; then
        $PAGE_EXTRACTOR_BIN $PAGE_EXTRACTOR_ARGS > $log_file 2>&1
        if [ "$?" != "0" ]; then
            echo "extract data error, gflags=$gflags_file"
            return 1
        else
            echo "extract data finished, gflags=$gflags_file"
            return 0
        fi
    elif [ "$#" == "1" ]; then
        gflags_file="$1"
        $PAGE_EXTRACTOR_BIN --flagfile=$gflags_file > $log_file 2>&1
        if [ "$?" != "0" ]; then
            echo "extract data error, gflags=$gflags_file"
            return 1
        else
            echo "extract data finished, gflags=$gflags_file"
            return 0
        fi
    elif [ "$#" == "3" ]; then
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
            $PAGE_EXTRACTOR_BIN --EXTRACTOR_input_tpls_file=$tpls_conf_file --EXTRACTOR_input_format=JSON --EXTRACTOR_input_file=$data_file --EXTRACTOR_output_file=$out_json_file > $log_file 2>&1
            #$PAGE_EXTRACTOR_BIN --EXTRACTOR_input_tpls_file=$tpls_conf_file --EXTRACTOR_input_format=JSON --EXTRACTOR_input_file=$data_file --EXTRACTOR_output_file=$out_json_file 
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
    fi 
}


CMDNAME=`basename "$0"`

if [ "$CMDNAME" = 'spider_start.sh' ];then
    _start_spider_ 
    sleep 2
    _info_spider_ 
fi

if [ "$CMDNAME" = 'spider_stop.sh' ];then
    _stop_spider_ 
fi

if [ "$CMDNAME" = 'spider_kill.sh' ];then
    _kill_spider_ 
fi

if [ "$CMDNAME" = 'spider_restart.sh' ];then
    _stop_spider_ 
    sleep 2
    _start_spider_ 
    sleep 2
    _info_spider_ 
fi

if [ "$CMDNAME" = 'spider_info.sh' ];then
    _info_spider_ 
fi

if [ "$CMDNAME" = 'dispatcher_start.sh' ];then
    _start_dispatcher_
    sleep 2
    _info_dispatcher_
fi

if [ "$CMDNAME" = 'dispatcher_stop.sh' ];then
    _stop_dispatcher_
fi

if [ "$CMDNAME" = 'dispatcher_kill.sh' ];then
    _kill_dispatcher_
fi

if [ "$CMDNAME" = 'dispatcher_restart.sh' ];then
    _stop_dispatcher_
    sleep 2
    _start_dispatcher_
    sleep 2
    _info_dispatcher_
fi

if [ "$CMDNAME" = 'dispatcher_info.sh' ];then
    _info_dispatcher_
fi

if [ "$CMDNAME" = "query_client.sh" ];then
    if [ "$#" == "0" ]; then
        _query_client_
    elif [ "$#" == "1" ]; then
        gflags_file="$1"
        _query_client_ "$gflags_file"
    elif [ "$#" == "2" ]; then
        server_addrs="$1"
        query_file="$2"
        _query_client_ "$server_addrs" "$query_file"
    else
        echo "Usage: $CMDNAME <server_addr> <server_port> <query_file>"
        exit 1
    fi
    
fi

if [ "$CMDNAME" = "search_list_client.sh" ];then
    if [ "$#" == "0" ]; then
        _search_list_client_ 
    elif [ "$#" == "1" ]; then
        gflags_file="$1"
        _search_list_client_ "$gflags_file"
    elif [ "$#" == "2" ]; then
        server_addrs="$1"
        search_list_input_json_file="$2"
        data_format="JSON"
        _search_list_client_ "$server_addrs" "$data_format" "$search_list_input_json_file"
    else
        echo "Usage: $CMDNAME [<server_addrs> <search_list_json_file>]|[<search_list_client.gflags>]"
        exit 1
    fi
fi

if [ "$CMDNAME" = "make_data_list.sh" ];then
    echo $#
    if [ "$#" != 3 ]; then
        echo "Usage: $CMDNAME <INC|ALL> <data_file_pattern> <data_list_outfile>"
        exit 1
    fi
    if [ "$1" != "INC" -a "$1" != "ALL" ]; then
        echo "Usage: $CMDNAME <INC|ALL>"
        exit 1
    fi

    flag="$1"
    file_pattern="$2"
    data_list_outfile="$3"
    
    echo $flag, $file_pattern, $data_list_outfile

    datafiles_n=`ls -lu $file_pattern 2>/dev/null | wc -l`
    datafiles=`ls -lu $file_pattern 2>/dev/null | awk '{print $NF}'` # 逆序 -lru
    if [ -z "$datafiles" ]; then
        echo "no data files exist"
        exit 1
    fi
    if [ "$flag" == "INC" ]; then
        let datafiles_n=$datafiles_n-1
        echo $datafiles_n
    fi

    # clear
    >$data_list_outfile

    i=0
    for file in $datafiles;
    do
        if [ "$i" -ge "$datafiles_n" ]; then
            break
        fi
        echo $file
        echo $file  >>$data_list_outfile

        let i=$i+1
    done
fi

if [ "$CMDNAME" = "extract_data.sh" ];then
    if [ "$#" == "0" ]; then
        _extract_data_
    elif [ "$#" == "1" ]; then
        gflags_file="$1"
        _extract_data_ "$gflags_file"
    elif [ "$#" == "3" ]; then
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
    else
        echo "Usage: $CMDNAME <tpls_conf_file> <data_list_file> <out_json_file>"
        exit 1
    fi
fi

if [ "$CMDNAME" = "extract_data.sh" ];then

fi
