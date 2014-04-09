#!/bin/sh

ulimit -c unlimited

export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/usr/local/thrift-0.9.1/lib

./src/service_spider --flagfile=conf/conf.gflags

