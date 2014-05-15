#!/bin/sh

. $HOME/.bash_profile

SCRIPT_PWD="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
export WORKDIR=`dirname $SCRIPT_PWD`
if [ -z "$WORKDIR" ]; then
    echo "WORKDIR is empty, set first!!"
    exit 1
fi
echo "WORKDIR=$WORKDIR"

#
#./scripts/spider_service_extract_data.sh src/extractor/input.json 1.json
#./scripts/spider_service_extract_baidu_search_list.sh src/extractor/input.json 2.json
#./scripts/spider_service_parse_baidu_search_list.sh 2.json 3.json
#./scripts/spider_service_gen_search_list_fetch_urls.sh 3.json output
#
#./sbin/spider_service_extract_data.sh input.json 1.json
#./sbin/spider_service_extract_baidu_search_list.sh input.json 2.json
#./sbin/spider_service_parse_baidu_search_list.sh 2.json 3.json
#./sbin/spider_service_gen_search_list_fetch_urls.sh 3.json output
#

today="`date +"%Y%m%d" -d "-0 days"`"
hour="`date +"%H" -d "-0 days"`"

DATATYPE='msearch-others_eng_results'
HDFS_INPUT_BASE_DIR="/home/hdp-ms/output/spider_service/${DATATYPE}/input"
#HDFS_INPUT_BASE_DIR="/home/hdp-ms/output/spider_service/${DATATYPE}/test"
HDFS_OUTPUT_BASE_DIR="/home/hdp-ms/output/spider_service/${DATATYPE}/output"

function _commit_files_()
{
    urls_file_prefix="$1"
    hdfs_output_dir="$2"

    parent_hdfs_output_dir=`dirname ${hdfs_output_dir}`
    hadoop fs -test -d ${parent_hdfs_output_dir}
    hdfs_output_dir_isexist="$?"
    if [ "$hdfs_output_dir_isexist" != "0" ]; then
        hadoop fs -mkdir ${parent_hdfs_output_dir}
    fi
    hadoop fs -test -d ${hdfs_output_dir}
    hdfs_output_dir_isexist="$?"
    if [ "$hdfs_output_dir_isexist" != "0" ]; then
        hadoop fs -mkdir ${hdfs_output_dir}
    fi

    commit_files=`ls ${urls_file_prefix}* 2>/dev/null`
    if [ -z "$commit_files" ]; then
        echo "no file ${urls_file_prefix}*"
        return 2
    fi
    has_error=0
    for file in $commit_files;
    do
        echo "committing $file to $hdfs_output_dir ..."
        hadoop fs -put $file $hdfs_output_dir
        if [ "$?" == "0" ]; then
            echo "commiting $file to $hdfs_output_dir success!"
        else
            echo "commiting $file to $hdfs_output_dir failed!"
            has_error=1
        fi
    done
    if [ "$has_error" == "1" ]; then
        echo "commit files failed!"
        return 1
    else
        echo "commit files success!"
        submit_hdfs_dir="${hdfs_output_dir}"
        touch _SUCCESS
        hadoop fs -put _SUCCESS $hdfs_output_dir
        echo "submit spider_service fetch request inputdir=${submit_hdfs_dir} outputdir=${HDFS_OUTPUT_BASE_DIR} DATATYPE=${DATATYPE}"
        curl "http://client38v.qss.zzbc2.qihoo.net:8080/submit_request.php?name=${DATATYPE}&inputdir=${submit_hdfs_dir}&outputdir=${HDFS_OUTPUT_BASE_DIR}"
        if [ "$?" == "0" ]; then
            echo "submit spider_service fetch request ok!"
            return 0
        else
            echo "submit spider_service fetch request failed!"
        fi
        return 1
    fi
}

CMDNAME=`basename "$0"`
if [ "$CMDNAME" = "spider_service_extract_data.sh" ];then
    if [ "$#" != "2" ]; then
        echo "Usage: $CMDNAME <input_file> <output_file>"
        exit 1
    else
        input_file="$1"
        output_file="$2"
    
        if [ ! -f "$input_file" ]; then
            echo "$input_file not exist or no regular file"
            exit 1
        fi
        ${WORKDIR}/bin/spider_service_extractor --EXTRACTOR_input_tpls_file=${WORKDIR}/conf/tpls.conf --EXTRACTOR_input_format=SPIDER_SERVICE_JSON --EXTRACTOR_input_file=${input_file} --EXTRACTOR_output_file=${output_file}
    fi
elif [ "$CMDNAME" = "spider_service_extract_baidu_search_list.sh" ];then
    if [ "$#" != "2" ]; then
        echo "Usage: $CMDNAME <input_file> <output_file>"
        exit 1
    else
        input_file="$1"
        output_file="$2"
    
        if [ ! -f "$input_file" ]; then
            echo "$input_file not exist or no regular file"
            exit 1
        fi
        ${WORKDIR}/bin/spider_service_extractor --EXTRACTOR_input_tpls_file=${WORKDIR}/conf/tpls.baidu.searchlist.conf --EXTRACTOR_input_format=SPIDER_SERVICE_JSON --EXTRACTOR_input_file=${input_file} --EXTRACTOR_output_file=${output_file}
    fi
elif [ "$CMDNAME" = "spider_service_parse_baidu_search_list.sh" ];then
    if [ "$#" != "2" ]; then
        echo "Usage: $CMDNAME <input_file> <output_file>"
        exit 1
    else
        input_file="$1"
        output_file="$2"
    
        if [ ! -f "$input_file" ]; then
            echo "$input_file not exist or no regular file"
            exit 1
        fi
        ${WORKDIR}/bin/spider_service_extractor --EXTRACTOR_input_format=BAIDU_SEARCH_LIST --EXTRACTOR_input_file=${input_file} --EXTRACTOR_output_file=${output_file}
    fi
elif [ "$CMDNAME" = "spider_service_gen_query_fetch_urls.sh" ];then
    if [ $# -ne 2 ]; then
        echo "Usage: $0 <query-file> <output_dir>"
        exit 1
    fi
    query_file=$1
    output_dir="$2"
    mkdir -p ${output_dir}
    ${SCRIPT_PWD}/spider_service_gen_fetch_urls.py QUERY ${query_file} ${output_dir}/baidu-fetch-querys-$today-$hour
    if [ "$?" == "0" ]; then
        echo "./scripts/spider_service_gen_fetch_urls.py QUERY ${query_file} ${output_dir}/baidu-fetch-querys-$today-$hour success"
        _commit_files_ "${output_dir}/baidu-fetch-querys-$today-$hour" "${HDFS_INPUT_BASE_DIR}/${today}/query/"
        if [ "$?" == "0" ]; then
            exit 0
        else
            exit 1
        fi
    else
        echo "failed"
        exit 1
    fi
elif [ "$CMDNAME" = "spider_service_gen_search_list_fetch_urls.sh" ];then
    if [ $# -ne 2 ]; then
        echo "Usage: $0 <search-list-file> <output_dir>"
        exit 1
    fi
    search_list_file=$1
    output_dir="$2"
    mkdir -p ${output_dir}
    ${SCRIPT_PWD}/spider_service_gen_fetch_urls.py URL ${search_list_file} ${output_dir}/baidu-fetch-urls-$today-$hour
    if [ "$?" == "0" ]; then
        echo "./scripts/spider_service_gen_fetch_urls.py URL ${search_list_file} ${output_dir}/baidu-fetch-urls-$today-$hour success"
        _commit_files_ "${output_dir}/baidu-fetch-urls-$today-$hour" "${HDFS_INPUT_BASE_DIR}/${today}/result/"
        if [ "$?" == "0" ]; then
            exit 0
        else
            exit 1
        fi
    else
        echo "failed"
        exit 1
    fi
fi

