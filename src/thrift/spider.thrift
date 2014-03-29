/**
 * The first thing to know about are types. The available types in Thrift are:
 *
 *  bool        Boolean, one byte
 *  byte        Signed byte
 *  i16         Signed 16-bit integer
 *  i32         Signed 32-bit integer
 *  i64         Signed 64-bit integer
 *  double      64-bit floating point value
 *  string      String
 *  binary      Blob (byte array)
 *  map<t1,t2>  Map from one type to another
 *  list<t1>    Ordered list of one type
 *  set<t1>     Set of unique elements of one type
 *
 * Did you also notice that Thrift supports C style comments?
 */

namespace cpp spider.webservice

struct SiteConfig {
  1: string host,
  2: optional map<string, string> userdata,
  3: optional map<string, string> extra_headers,
}

struct HttpRequest {
  1: string url,
  2: optional string userdata,
}

service SpiderWebService {
   i32 setConfig(1:SiteConfig site_cfg),
   i32 submit(1:HttpRequest rqst),
   i32 submit_url(1:string url),
}


