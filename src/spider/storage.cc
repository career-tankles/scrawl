
#include <assert.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <glog/logging.h>
#include <gflags/gflags.h>

#include "storage.h"
#include "conf.h"
#include "HttpParser.h"
#include "cJSON.h"

namespace spider {
namespace store {

class storage 
{
public:
    storage() 
      : data_prefix_(FLAGS_STORE_file_dir), cur_file_(""), fd_(-1), file_no_(-1), last_switch_file_time_hour_(0)
    {
        //pthread_mutex_init(&mutex_, NULL);
        if(FLAGS_STORE_file_swith_time_sec <= 0)
            FLAGS_STORE_file_swith_time_sec = 3600;
    }
    ~storage() {
        //pthread_mutex_destroy(&mutex_);
    }

    void init(std::string prefix) {
        
        data_prefix_ = prefix;

        struct stat stat_buf;
        char buf[256];
        while(true) {
            file_no_ ++;
            if(file_no_ == FLAGS_STORE_file_max_num) {
                LOG(INFO)<<"STORAGE file reach max "<<FLAGS_STORE_file_max_num<<", use the first one";
                file_no_ = 0;
                //sprintf(buf, "%s-%03d", data_prefix_.c_str(), file_no_);
                sprintf(buf, "%s-%03d.data", data_prefix_.c_str(), file_no_);
                break;
            }
            //sprintf(buf, "%s-%03d", data_prefix_.c_str(), file_no_);
            sprintf(buf, "%s-%03d.data", data_prefix_.c_str(), file_no_);
            if(-1 == stat(buf, &stat_buf)) {
                break; 
            }
            if(stat_buf.st_size <= 0)
                break;
            LOG(INFO)<<"STORAGE file: "<<buf<<" exist!";
        }
        file_no_ --;    // file_no表示当前文件序号
        LOG(INFO)<<"STORAGE init file: "<<buf;
    }

    int write(const char* buf, size_t len) {
        switch_file();      // 根据需要切换文件
        assert(fd_ > 0);
        return write(fd_, buf, len);
    }

private:

    int write(int fd, const char* buf, size_t len)
    {
        assert(fd > 0);
        assert(buf != NULL);
        size_t left = len;
        while(left > 0) {
            ssize_t n = ::write(fd, buf, len);
            if(n < 0)
                return -1;
            left -= n;
        }
        return 0;
    }

    int new_fileno() {
        //pthread_mutex_lock(&mutex_);
        file_no_ = (file_no_+1)%FLAGS_STORE_file_max_num;
        int fileno = file_no_;
        //pthread_mutex_unlock(&mutex_);
        return fileno;
    }

    int stat(std::string filename, struct stat* stat_buf) {
        assert(stat_buf);
        int res = ::stat(filename.c_str(), stat_buf);
        if(res == -1) {
            //LOG(ERROR)<<"STORAGE stat file error "<<filename<<" errno="<<errno<<" errstr="<<strerror(errno);
        }
        return res;
    }

    int fstat(int fd, struct stat* stat_buf) {
        assert(fd > 0);
        assert(stat_buf);
        int res = ::fstat(fd, stat_buf);
        if(res == -1) {
            //LOG(ERROR)<<"STORAGE fstat file error "<<fd <<" errno="<<errno<<" errstr="<<strerror(errno);
        }
        return res;
    }

    int switch_file() {

        time_t now = time(NULL);
        if(fd_ == -1) {
            int new_fno = new_fileno();
            char buf[256] = {0};
            //sprintf(buf, "%s-%03d", data_prefix_.c_str(), new_fno);
            sprintf(buf, "%s-%03d.data", data_prefix_.c_str(), new_fno);
            cur_file_ = buf;
            fd_ = open(buf);
            last_switch_file_time_hour_ = now/FLAGS_STORE_file_swith_time_sec;
            return 1;
        } else {
            struct stat stat_buf;
            int res = fstat(fd_, &stat_buf);
            if(res == -1)
                exit(1);
            if(stat_buf.st_size >= FLAGS_STORE_file_max_size || (stat_buf.st_size > 0 && now/FLAGS_STORE_file_swith_time_sec != last_switch_file_time_hour_ )) {
                // 切换新文件
                close(fd_);
                int new_fno = new_fileno();
                char buf[256] = {0};
                sprintf(buf, "%s-%03d.data", data_prefix_.c_str(), new_fno);
                LOG(INFO)<<"STORAGE new file "<<stat_buf.st_size<<" "<<FLAGS_STORE_file_max_size<<" "<<last_switch_file_time_hour_<<" "<<buf;
                cur_file_ = buf;
                fd_ = open(buf);
                last_switch_file_time_hour_ = now/FLAGS_STORE_file_swith_time_sec;
                return 1;
            }
        }
        return 0;
    }
 
    int open(const char* filename)
    {
        //int fd = ::open(filename, O_WRONLY|O_CREAT|O_EXCL, 0777);
        int fd = ::open(filename, O_WRONLY|O_CREAT, 0777);
        if(fd < 0) {
            LOG(INFO)<<"STORAGE open failed "<<filename<<" "<<errno<<"-"<<strerror(errno);
            return -1;
        }
        return fd;
    }

    int close(int fd)
    {
        assert(fd > 0);
        ::fsync(fd);
        ::close(fd);
        return 0;
    }
    
private:
    std::string data_prefix_;

    int fd_;                    // 当前打开fd
    std::string cur_file_;      // 当前打开文件名

    int file_no_;               // 文件编号
    //pthread_mutex_t mutex_;
    time_t last_switch_file_time_hour_;

};

struct PageStorageArgs {
    int num;
    std::string data_dir;
    PageStorage* page_storage;
};

class _PageStorage_ {
public:
    void operator()(void* args) {
        // 参数: 目录 + 文件前缀 + 序号
        _io_process_(args);

    }

    void _io_process_(void* args) {
        PageStorageArgs* p = (PageStorageArgs*)args;
        assert(p);

        PageStorage* page_storage = (PageStorage*)p->page_storage;
        assert(page_storage);
        std::string data_dir = p->data_dir;
        int threadnum = p->num;

        char buf[256] = {0};
        sprintf(buf, "%s/spider-%02d", data_dir.c_str(), threadnum);

        storage storage_;
        storage_.init(std::string(buf));
    
        HttpParser parser;    // HTTP解析器

        struct http_result_t* result = NULL;
        while(page_storage->is_running_) {
            result = page_storage->http_result_.pop();
            if(result == NULL) {
                usleep(FLAGS_STORE_nores_usleep);
                continue;
            }
            std::string url = result->url;
            std::string data;
            int res = http_result_to_str(parser, result, data);
            if(res != 0) {
                LOG(ERROR)<<"STORAGE http_result_to_str failed ";
                continue;
            }
            res = storage_.write(data.c_str(), data.size());
            if (res != 0) {
                LOG(ERROR)<<"STORAGE write failed ";
                exit(-1);
            }
            LOG(INFO)<<"STORAGE save "<<url<<" "<<data.size()<<" bytes";
            usleep(FLAGS_STORE_usleep);
        }
        delete p;
        p = NULL;
    }

    void print(http_result_t* result) {
        static int n = 0;
        std::cerr<<"NUM="<<n++<<std::endl;
        std::cerr<<"url="<<result->url<<std::endl;
        std::cerr<<"submit_time="<<result->submit_time<<std::endl;
        std::cerr<<"write_end_time="<<result->write_end_time<<std::endl;
        std::cerr<<"recv_begin_time="<<result->recv_begin_time<<std::endl;
        std::cerr<<"recv_end_time="<<result->recv_end_time<<std::endl;
        std::cerr<<"scrawltime="<<result->scrawltime<<std::endl;
        std::cerr<<"state="<<result->state<<std::endl;
        std::cerr<<"fetch_ip="<<result->fetch_ip<<std::endl;
        //std::cerr<<"http_page_data="<<result->http_page_data<<std::endl;
        std::cerr<<"http_page_data_len="<<result->http_page_data_len<<std::endl;
        std::cerr<<std::endl;
        std::cerr<<std::endl;
    }
    
    int http_result_to_str(HttpParser& parser, http_result_t*& result, std::string& data) {
        assert(result);

        http_request_t* rqst = NULL;
        Res* res = NULL; 
        std::string userdata = "";

        if(result->rqst) rqst = result->rqst;
        if(result->res) {
             res = result->res;
             userdata = res->userdata;
        }

        // http_parser解析网页
        std::string headers, body;
        parser.parse(result->http_page_data, &headers, &body);


        data = "";
        if(FLAGS_STORE_format == "JSON") {
            // JSON
            cJSON* jnew_root = cJSON_CreateObject();
            cJSON* juserinfo = cJSON_CreateObject();

            cJSON_AddStringToObject(juserinfo, "url", result->url.c_str());
            cJSON_AddStringToObject(juserinfo, "host", result->host.c_str());
            cJSON_AddStringToObject(juserinfo, "status", result->state==http_result_t::HTTP_PAGE_OK?"OK":"ERROR");
            cJSON_AddNumberToObject(juserinfo, "errno", result->error_code);
            cJSON_AddStringToObject(juserinfo, "server", result->fetch_ip.c_str());
            cJSON_AddNumberToObject(juserinfo, "submit_time", result->submit_time);
            cJSON_AddNumberToObject(juserinfo, "write_end_time", result->write_end_time);
            cJSON_AddNumberToObject(juserinfo, "recv_begin_time", result->recv_begin_time);
            cJSON_AddNumberToObject(juserinfo, "recv_end_time", result->recv_end_time);
            cJSON_AddStringToObject(juserinfo, "scrawltime", result->scrawltime.c_str());
            cJSON_AddNumberToObject(juserinfo, "read_data_len", result->http_page_data_len);
            if(userdata.size() > 0)
                cJSON_AddStringToObject(juserinfo, "userdata", userdata.c_str());

            cJSON_AddItemToObject(jnew_root, "info", juserinfo);

            cJSON_AddNumberToObject(jnew_root, "headers_size", headers.size());
            cJSON_AddStringToObject(jnew_root, "headers", headers.c_str());

            cJSON_AddNumberToObject(jnew_root, "body_size", body.size());
            cJSON_AddStringToObject(jnew_root, "body", body.c_str());

            data = cJSON_PrintUnformatted(jnew_root);
            data += "\r\n";
            cJSON_Delete(jnew_root);
        } else if(FLAGS_STORE_format == "HTTP") {
            int n = 0;
            char buf[1024*1024];
            n += snprintf(buf+n, sizeof(buf)-n, "URL: %s\r\n", result->url.c_str());
            n += snprintf(buf+n, sizeof(buf)-n, "STATE: %s\r\n", result->state==http_result_t::HTTP_PAGE_OK?"OK":"ERROR");
            n += snprintf(buf+n, sizeof(buf)-n, "Server: %s\r\n", result->fetch_ip.c_str());
            n += snprintf(buf+n, sizeof(buf)-n, "submit_time: %d\r\n", result->submit_time);
            n += snprintf(buf+n, sizeof(buf)-n, "write_end_time: %d\r\n", result->write_end_time);
            n += snprintf(buf+n, sizeof(buf)-n, "recv_begin_time: %d\r\n", result->recv_begin_time);
            n += snprintf(buf+n, sizeof(buf)-n, "recv_end_time: %d\r\n", result->recv_end_time);
            n += snprintf(buf+n, sizeof(buf)-n, "scrawltime: %s\r\n", result->scrawltime.c_str());
            if(userdata.size() > 0)
                n += snprintf(buf+n, sizeof(buf)-n, "userdata: %s\r\n", userdata.c_str());
            n += snprintf(buf+n, sizeof(buf)-n, "read_data_len: %d\r\n", result->http_page_data_len);
            n += snprintf(buf+n, sizeof(buf)-n, "headers_size: %d\r\n", headers.size());
            n += snprintf(buf+n, sizeof(buf)-n, "body_size: %d\r\n", body.size());
    
            n += snprintf(buf+n, sizeof(buf)-n, "\r\n");
            data += std::string(buf, n);
            data += headers + "\r\n";
            data += body + "\r\n";
            data += "\r\n";
        } else {
            LOG(ERROR)<<"Unknown STORE_format, JSON and HTTP is support";
        }
        
        LOG(INFO)<<"STORAGE http response "<<result->url<<" "<<(result->state==http_result_t::HTTP_PAGE_OK?"OK":"ERROR")<<" "<<FLAGS_STORE_format<<" pagelen="<<result->http_page_data_len<<" body_size="<<body.size()<<" len="<<data.size();

        // 释放资源
        if(res) { delete res;  res = NULL; }
        if(rqst){ delete rqst; rqst = NULL;}
        if(result) { delete result; result = NULL; }

        return 0;
    }

private:
};

PageStorage::PageStorage()
  : http_result_(FLAGS_STORE_rslt_queue_size)
{
}

PageStorage::~PageStorage()
{
}

int PageStorage::start() {
    is_running_ = true;
    LOG(INFO)<<"STORAGE start "<<FLAGS_STORE_threads<<" threads";
    for(int i=0; i<FLAGS_STORE_threads; i++) {

        PageStorageArgs* args = new PageStorageArgs;
        args->num = i;
        args->data_dir = FLAGS_STORE_file_dir;
        args->page_storage = this;
        threadpool_.create_thread(_PageStorage_(), (void*)args);
    }
    return 0;
}

int PageStorage::stop() {
    is_running_ = false;
    threadpool_.wait_all();
    LOG(INFO)<<"STORAGE stop "<<FLAGS_STORE_threads<<" threads";
    return 0;
}

int PageStorage::submit(struct http_result_t* result) {
    if(!result) return -1;
    http_result_.push_wait(result);
    return 0;
}

}
}

