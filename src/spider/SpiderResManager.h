
#include <time.h>
#include <string>
#include <map>

#include "message.h"
#include "simple_queue.h"

#include "dns.h"
#include "storage.h"
#include "download.h"
#include "TimeWait.h"

using namespace spider::message;
using namespace spider::dns;
using namespace spider::download;
using namespace spider::store;


namespace spider {
  namespace scheduler {

class SpiderResManager
{
public:
    SpiderResManager();
    ~SpiderResManager();

    int load(std::string& cfg_file) { return 0; }
    int start();
    int stop();
    int run_loop();

    int submit(Res* res);    // 提交一个抓取资源

private:
    int svc();

private:
    std::map<std::string, boost::shared_ptr<Website> > sites_map_;          // 每个host对应一项

    ResList res_list_;
    ptr_queue<WebsiteConfig> website_cfg_;

    bool is_running_ ;

    boost::shared_ptr<dns::DNS> dnsmanager;
    boost::shared_ptr<TimeWait> timewaiter;
    boost::shared_ptr<Downloader> downloader;
    boost::shared_ptr<PageStorage> storage;
    
};

}
}
