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
    while((n=read(fd, buf+len, sizeof(buf)-len-1)) > 0) {
        len += n;
    }
    close(fd);

    buf[len] = '\0';
    data = std::string(buf, len);
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


#endif

