#!/usr/bin/env python
#coding=utf-8

# type == QUERY 
# intput-format:
# one query each line

# type == URL
# intput-format:
# url userdata

# output-format:
# url:http://m.baidu.com/s?word=xxxxxx\tdatatype:msearch-other_engine_results\tuser_agent:User-Agent:Mozilla/5.0 (Linux; U; Android 2.3; en-us) AppleWebKit/999+ (KHTML, like Gecko) Safari/999.9\topts:0-xxxxxxxxxxxxxxxx-query

DATATYPE = 'msearch-others_eng_results'
USERAGENT = 'Mozilla/5.0 (Linux; U; Android2.3; en-us) AppleWebKit/999+ (KHTML, like Gecko) Safari/999.9'

import sys
import md5
import urllib

def query_to_baidu_url(query):
    data = { 'word': query }
    encoded_data = urllib.urlencode(data)
    url = 'http://m.baidu.com/s?' + encoded_data
    md5_key = md5.new()
    md5_key.update(query)
    userdata = '%s-%s-%s' % ('0', md5_key.hexdigest(), encoded_data.replace('word=', ''))
    line = 'url:%s\tdatatype:%s\tuser_agent:User-Agent:%s\topts:%s\n' % (url, DATATYPE, USERAGENT, userdata)
    return line

# 将query_file中的query按照limit大小生成多个url文件(limit<=1w)
def load_querys(query_file, output_file_prefix, limit=10000):
    try:
        i = 0
        file_no = 0
        outfd = None
        for line in file(query_file):
            if i >= limit:
                i = 0
            if i <= 0:
                if outfd:
                    outfd.close()
                print 'new file %s-%05d' % (output_file_prefix, file_no)
                outfd = open('%s-%05d' % (output_file_prefix, file_no), 'w')
                file_no += 1
            line = line.strip()
            if len(line) <= 0 or line.startswith('#'):
                continue
            l = line.split('\t')
            if len(l) < 2:
                continue
            query = l[0]
            out_line = query_to_baidu_url(query)
            outfd.write(out_line)
            i += 1
        if outfd:
            outfd.close()
        return True
    except Exception, e:
        print 'Exception load_querys %s' % str(e)
    return False

# 将query_file中数据按照limit大小生成多个url文件(limit<=1w)
def load_urls(url_file, output_file_prefix, limit=10000):
    try:
        i = 0
        file_no = 0
        outfd = None
        for line in file(url_file):
            if i >= limit:
                i = 0
            if i <= 0:
                if outfd:
                    outfd.close()
                outfd = open('%s-%05d' % (output_file_prefix, file_no), 'w')
                file_no += 1
            line = line.strip()
            if len(line) <= 0 or line.startswith('#'):
                continue
            l = line.split('\t')
            out_line = ''
            if len(l) == 2:
                url, userdata = l
                out_line = 'url:%s\tdatatype:%s\tuser_agent:User-Agent:%s\topts:%s\n' % (url, DATATYPE, USERAGENT, userdata)
            else:
                url = line
                out_line = 'url:%s\tdatatype:%s\tuser_agent:User-Agent:%s\t\n' % (url, DATATYPE, USERAGENT)
            outfd.write(out_line)
            i += 1
        if outfd:
            outfd.close()
        return True
    except Exception, e:
        print 'Exception load_querys %s' % str(e)
    return False

if len(sys.argv) != 4:
    print 'a-Usage: %s QUERY|URL input_file output_file_prefix'
    sys.exit(1)


type = sys.argv[1]
input_file = sys.argv[2]
output_file = sys.argv[3]

if type == "QUERY":
    load_querys(input_file, output_file)
    sys.exit(0)
elif type == "URL":
    ret = load_urls(input_file, output_file)
    sys.exit(0)
else:
    print 'Usage: %s QUERY|URL input_file output_file_prefix'
    sys.exit(1)

