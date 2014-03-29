
#ifndef __CONFIG_H__
#define __CONFIG_H__

#include <gflags/gflags.h>

DECLARE_int32(RES_url_list_queue_size); 
DECLARE_int32(RES_usleep);

DECLARE_int32(DNS_threads);
DECLARE_int32(DNS_rqst_queue_size);
DECLARE_int32(DNS_rslt_queue_size);
DECLARE_int32(DNS_usleep);
DECLARE_int32(DNS_nores_usleep);
DECLARE_int32(DNS_error_retry_time);


DECLARE_int32(DOWN_threads);
DECLARE_int32(DOWN_rqst_queue_size);
DECLARE_int32(DOWN_rslt_queue_size);
DECLARE_int32(DOWN_http_page_maxsize);
DECLARE_int32(DOWN_usleep);
DECLARE_int32(DOWN_nores_usleep);
DECLARE_int32(DOWN_clock_interval_sec);
DECLARE_int32(DOWN_clock_interval_us);

DECLARE_int32(STORE_threads);
DECLARE_int32(STORE_rslt_queue_size);
DECLARE_int32(STORE_usleep);
DECLARE_int32(STORE_nores_usleep);
DECLARE_int32(STORE_file_max_num);
DECLARE_int64(STORE_file_max_size);
DECLARE_string(STORE_file_prefix);


DECLARE_int32(WAITER_threads);
DECLARE_int32(WAITER_usleep);
DECLARE_int32(WAITER_nores_usleep);
DECLARE_int32(WAITER_rqst_queue_size);
DECLARE_int32(WAITER_rslt_queue_size);


#endif

