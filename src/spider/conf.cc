
#include "conf.h"
#include <glog/logging.h>

DEFINE_int32(SERVER_thrift_port, 9090, ""); 

DEFINE_int32(RES_url_list_queue_size, 20000, ""); 
DEFINE_int32(RES_usleep, 100, "");
DEFINE_int32(RES_add_res_each_loop, 500, "");

DEFINE_int32(DNS_threads, 1, "");
DEFINE_int32(DNS_rqst_queue_size, 10000, "");
DEFINE_int32(DNS_rslt_queue_size, 20000, "");
DEFINE_int32(DNS_usleep, 100, "");
DEFINE_int32(DNS_nores_usleep, 200, "");
DEFINE_int32(DNS_error_retry_time_sec, 3600, "");
DEFINE_int32(DNS_update_time_sec, 86400, "");
DEFINE_int32(DNS_retry_max, 3, "");

DEFINE_int32(DOWN_threads, 1, "");
DEFINE_int32(DOWN_rqst_queue_size, 1000, "");
DEFINE_int32(DOWN_rslt_queue_size, 2000, "");
DEFINE_int32(DOWN_http_page_maxsize, 100*1024, "");
DEFINE_int32(DOWN_err_retry_max, 3, "");
//DEFINE_int32(DOWN_usleep, 10, "");
//DEFINE_int32(DOWN_nores_usleep, 100, "");
DEFINE_int32(DOWN_clock_interval_sec, 0, "");
DEFINE_int32(DOWN_clock_interval_us, 1000, "");
DEFINE_int32(DOWN_read_timeout_ms, 5000, "");
DEFINE_int32(DOWN_write_timeout_ms, 1000, "");
DEFINE_int32(DOWN_defalut_fetch_interval_sec, 10, "");
DEFINE_string(DOWN_UserAgent, "Mozilla/5.0 (Linux; U; Android 2.3; en-us) AppleWebKit/999+ (KHTML, like Gecko) Safari/999.9", "");

DEFINE_int32(STORE_threads, 1, "");
DEFINE_int32(STORE_rslt_queue_size, 5000, "");
DEFINE_int32(STORE_usleep, 100, "");
DEFINE_int32(STORE_nores_usleep, 200, "");
DEFINE_int32(STORE_file_max_num, 1000, "");
DEFINE_uint64(STORE_file_max_size, 0x100000000, "");
DEFINE_string(STORE_file_dir, "/tmp/spider/data", "must exist");
DEFINE_string(STORE_format, "JSON", "");

DEFINE_int32(WAITER_threads, 1, "");
//DEFINE_int32(WAITER_usleep, 100, "");
//DEFINE_int32(WAITER_nores_usleep, 200, "");
DEFINE_int32(WAITER_rqst_queue_size, 10000, "");
DEFINE_int32(WAITER_rslt_queue_size, 10000, "");

void output_config() {
    LOG(INFO)<<"RES_url_list_queue_size="<<FLAGS_RES_url_list_queue_size;
    LOG(INFO)<<"RES_usleep="<<FLAGS_RES_usleep;
    LOG(INFO)<<"DNS_threads="<<FLAGS_DNS_threads;
    LOG(INFO)<<"DNS_rqst_queue_size="<<FLAGS_DNS_rqst_queue_size;
    LOG(INFO)<<"DNS_rslt_queue_size="<<FLAGS_DNS_rslt_queue_size;
    LOG(INFO)<<"DNS_usleep="<<FLAGS_DNS_usleep;
    LOG(INFO)<<"DNS_nores_usleep="<<FLAGS_DNS_nores_usleep;
    LOG(INFO)<<"DNS_error_retry_time_sec="<<FLAGS_DNS_error_retry_time_sec;
    LOG(INFO)<<"DNS_update_time_sec="<<FLAGS_DNS_update_time_sec;
    LOG(INFO)<<"DNS_retry_max="<<FLAGS_DNS_retry_max;
    LOG(INFO)<<"DOWN_threads="<<FLAGS_DOWN_threads;
    LOG(INFO)<<"DOWN_rqst_queue_size="<<FLAGS_DOWN_rqst_queue_size;
    LOG(INFO)<<"DOWN_rslt_queue_size="<<FLAGS_DOWN_rslt_queue_size;
    LOG(INFO)<<"DOWN_http_page_maxsize="<<FLAGS_DOWN_http_page_maxsize;
    LOG(INFO)<<"DOWN_clock_interval_sec="<<FLAGS_DOWN_clock_interval_sec;
    LOG(INFO)<<"DOWN_clock_interval_us="<<FLAGS_DOWN_clock_interval_us;
    LOG(INFO)<<"DOWN_read_timeout_ms="<<FLAGS_DOWN_read_timeout_ms;
    LOG(INFO)<<"DOWN_write_timeout_ms="<<FLAGS_DOWN_write_timeout_ms;
    LOG(INFO)<<"DOWN_defalut_fetch_interval_sec="<<FLAGS_DOWN_defalut_fetch_interval_sec;
    LOG(INFO)<<"DOWN_UserAgent="<<FLAGS_DOWN_UserAgent;
    LOG(INFO)<<"STORE_threads="<<FLAGS_STORE_threads;
    LOG(INFO)<<"STORE_rslt_queue_size="<<FLAGS_STORE_rslt_queue_size;
    LOG(INFO)<<"STORE_usleep="<<FLAGS_STORE_usleep;
    LOG(INFO)<<"STORE_nores_usleep="<<FLAGS_STORE_nores_usleep;
    LOG(INFO)<<"STORE_file_max_num="<<FLAGS_STORE_file_max_num;
    LOG(INFO)<<"STORE_file_max_size="<<FLAGS_STORE_file_max_size;
    LOG(INFO)<<"STORE_file_dir="<<FLAGS_STORE_file_dir;
    LOG(INFO)<<"STORE_format="<<FLAGS_STORE_format;

    LOG(INFO)<<"WAITER_threads="<<FLAGS_WAITER_threads;
    LOG(INFO)<<"WAITER_rqst_queue_size="<<FLAGS_WAITER_rqst_queue_size;
    LOG(INFO)<<"WAITER_rslt_queue_size="<<FLAGS_WAITER_rslt_queue_size;
}
