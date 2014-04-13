#!/bin/sh

ulimit -c unlimited

BIN=./src/spider/service_spider

export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:libs/thrift-0.9.1/lib:libs/boost-1.50.0/lib:libs/gflags-1.2/lib:libs/glog-0.3.3/lib:libs/libevent-1.4.10/lib

nohup $BIN --flagfile=conf/conf.gflags > spider.log 2>&1 &


