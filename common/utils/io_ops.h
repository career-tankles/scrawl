#ifndef _IO_OPS_H_
#define _IO_OPS_H_

#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>

#include <string>
#include <glog/logging.h>
#include <gflags/gflags.h>

// 一次性全部读取到内容(小文件<1M)
static int load(const char* file, std::string& data) {
    int fd = open(file, O_RDONLY);
    if(fd == -1) {
        fprintf(stderr, "Error: %s %d-%s\n", file, errno, strerror(errno));
        return -1;
    }

    int n = 0, len = 0;
    char buf[1024*1024];
    //while((n=read(fd, buf+len, sizeof(buf)-len-1)) > 0) {
    //    len += n;
    //}
    while((n=read(fd, buf, sizeof(buf)-1)) > 0) {
        data += std::string(buf, n);
    }
    close(fd);

    //buf[len] = '\0';
    //data = std::string(buf, len);
    return 0;
}


static int io_append(int fd, const char* s, size_t n) {
    if(fd < 0) {
        LOG(ERROR)<<"IO invalid fd "<<fd;
        return -2;
    }
    int ret = lseek(fd, 0L, SEEK_END);
    if(ret < 0) {
        LOG(ERROR)<<"IO fseek failed "<<errno<<" "<<strerror(errno);
        return -3;
    }
    int left = n;
    const char* p = s;
    while(left > 0) {
        int n = write(fd, p, left);
        if(n < 0) {
            LOG(ERROR)<<"IO write data failed "<<errno<<" "<<strerror(errno);
            break;
        }
        left -= n;
        p += n;
    }
    return left==0?0:-1;
}

static int io_append(int fd, std::string& s) {
    return io_append(fd, s.c_str(), s.size());
}

static void example_cb(char* data, size_t len, void* args = NULL)
{
    static int i = 0;
    printf("%d - data:%s len:%d\n", i++, data, len);
}


int load_file(const char* szfile, void (*cb)(char* data, size_t len, void* args), void* args=NULL)
{
    int fd = open(szfile, O_RDONLY);
    if(fd == -1) {
        fprintf(stderr, "open file %s failed, %d-%s\n", szfile, errno, strerror(errno));
        return -1;
    }
    char buf[4*1024*1024+1] = {0};
    const ssize_t max_len = sizeof(buf)-1;
    int offset = 0;
    while(true) {
        ssize_t r = read(fd, buf+offset, max_len-offset);
        if(r == 0) {
            break;
        } else if (r == -1) {
            fprintf(stderr, "read file %s failed, %d-%s\n", szfile, errno, strerror(errno));
            return -1;
        }
        offset += r;
        *(buf+offset) = '\0';

        char* pbegin = buf + strspn(buf , "\r\n"); 
        char* plineend = NULL;
        while(pbegin < buf+offset) {
            plineend = strpbrk(pbegin, "\r\n");
            if(!plineend) break;

            *plineend++ = '\0';
            if(cb) 
                cb(pbegin, plineend-pbegin-1, args);
            else
                printf("line-len %d\n", plineend-1-pbegin);
            pbegin = plineend  + strspn(plineend  , "\r\n");
        }
        if(pbegin < buf+offset) { // plineend==NULL, 剩余内容，没有换行
            memmove(buf, pbegin, buf+offset-pbegin);
            offset = buf+offset-pbegin;
        } else {
            offset = 0;
        }
    }
    if(offset > 0) {
        char* pbegin = buf + strspn(buf , "\r\n");
        if(pbegin < buf+offset) { // 存在有效数据
            if(cb) 
                cb(buf, offset, args);
            else
                printf("line-len %d\n", offset);
        }
    }

}


#endif

