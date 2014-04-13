
#include "cfg_tpl.h"

DEFINE_string(EXTRACTOR_input_tpls_file, "tpls.conf", "");

int main()
{
    std::map<std::string, struct cfg_tpl_host> maps_tpls;
    int ret = load_tpls(FLAGS_EXTRACTOR_input_tpls_file, maps_tpls) ;
    assert(ret == 0);


}
