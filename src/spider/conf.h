
#ifndef __CONFIG_H__
#define __CONFIG_H__

#include <gflags/gflags.h>

void output_config() ;

DECLARE_int32(SERVER_thrift_port);
DECLARE_int32(SERVER_thrift_threadnum);

DECLARE_int32(RES_url_list_queue_size); 
DECLARE_int32(RES_usleep);
DECLARE_int32(RES_add_res_each_loop);
DECLARE_string(RES_dump_file);

DECLARE_int32(DNS_threads);
DECLARE_int32(DNS_rqst_queue_size);
DECLARE_int32(DNS_rslt_queue_size);
DECLARE_int32(DNS_usleep);
DECLARE_int32(DNS_nores_usleep);
DECLARE_int32(DNS_error_retry_time_sec);
DECLARE_int32(DNS_update_time_sec);
DECLARE_int32(DNS_retry_max);

DECLARE_int32(DOWN_threads);
DECLARE_int32(DOWN_rqst_queue_size);
DECLARE_int32(DOWN_rslt_queue_size);
DECLARE_int32(DOWN_http_page_maxsize);
DECLARE_int32(DOWN_err_retry_max);
//DECLARE_int32(DOWN_usleep);
//DECLARE_int32(DOWN_nores_usleep);
DECLARE_int32(DOWN_clock_interval_sec);
DECLARE_int32(DOWN_clock_interval_us);
DECLARE_int32(DOWN_read_timeout_ms);
DECLARE_int32(DOWN_write_timeout_ms);
DECLARE_int32(DOWN_defalut_fetch_interval_sec);
DECLARE_string(DOWN_UserAgent);

DECLARE_int32(STORE_threads);
DECLARE_int32(STORE_rslt_queue_size);
DECLARE_int32(STORE_usleep);
DECLARE_int32(STORE_nores_usleep);
DECLARE_int32(STORE_file_max_num);
DECLARE_uint64(STORE_file_max_size);
DECLARE_string(STORE_file_dir);
DECLARE_string(STORE_format);

DECLARE_int32(WAITER_threads);
//DECLARE_int32(WAITER_usleep);
//DECLARE_int32(WAITER_nores_usleep);
DECLARE_int32(WAITER_rqst_queue_size);
DECLARE_int32(WAITER_rslt_queue_size);


#endif

