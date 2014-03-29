#include <assert.h>

#include "uri.h"

int main()
{
      uripp::uri u("http://mydomain.com/a/b/c?id=12345&s=x%20y");
      std::cout<<u<<std::endl;
      uripp::uri u2("http://m.baidu.com:80/ssid=0/from=0/bd_page_type=1/uid=0/baiduid=4880808961BE15D43033D05B160470A8/pu=sz%40224_220%2Cta%40middle___3_537/baiduid=4880808961BE15D43033D05B160470A8/w=0_10_%E9%98%BF%E5%8D%A1%E5%8D%A1/t=wap/l=0/tc?ref=www_colorful&lid=8507467953314498838&order=6&vit=osres&tj=www_normal_6_0_10&sec=37276&di=90cfd3ca2823e648&bdenc=1&nsrc=IlPT2AEptyoA_yixCFOxXnANedT62v3IEQGG_yFV_zK8jkWih0rgHtkfEFXgKHaIJoCb9jDKth5IuHKi_m_");
      std::cout<<u2<<std::endl;

      std::cout<<u.scheme().string()<<std::endl;
      std::cout<<u.authority()<<std::endl;
      std::cout<<u.authority().host()<<std::endl;
      std::cout<<u.path()<<std::endl;
      std::cout<<u.query()<<std::endl;
      std::cout<<u.path().absolute()<<std::endl;
      uripp::path::const_iterator it = u.path().begin();
      assert(*it == "a" && *++it == "b" && *++it == "c" && ++it == u.path().end());
      int id;
      std::string s;
      bool is_null;
      assert(u.query().find("id", id, is_null) && id == 12345 && !is_null);
      assert(u.query().find("s", s, is_null) && s == "x y" && !is_null);
      assert(!u.query().find("foo", s, is_null));
 
    return 0;
}
