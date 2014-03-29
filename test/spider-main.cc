#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <glog/logging.h>
#include <gflags/gflags.h>
#include "config.h"
#include "SpiderResManager.h"

using namespace spider::scheduler;

void load(std::string file, SpiderResManager& res_manager) {
    int fd = open(file.c_str(), O_RDONLY);
    if(fd == -1) return ;
    char buf[1024*1024];
    ssize_t n = 0;
    ssize_t left = 0;
    while((n=read(fd, buf+left, sizeof(buf)-left)) > 0)
    {
        char* p = buf;
        char* begin = buf;
        left += n;
        while(p < buf+left){
            if(*p == '\r' || *p == '\n'){
                //std::cout<<std::string(begin, p-begin)<<std::endl;
                Res* res = new Res;
                res->url = std::string(begin, p-begin);
                res_manager.submit(res);
                while(p<buf+left) {
                    if(*p != '\r' && *p != '\n')
                        break;
                    p++;
                }
                begin = p;
                continue;
            }
            p++;
        }
        if(begin != buf && p > begin)
            memmove(buf, begin, p-begin);
    }
}

int main(int argc, char* argv[])
{
    google::ParseCommandLineFlags(&argc, &argv, true);

    // Initialize Google's logging library.
    google::InitGoogleLogging(argv[0]);

    SpiderResManager res_manager;
    res_manager.start();
    fprintf(stderr, "load..\n");
    load("urls.txt", res_manager);   
    res_manager.run_loop();
    res_manager.stop();

    return 0;
}

