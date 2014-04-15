#!/usr/bin

PWD=`pwd`

export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$PWD/libs/thrift-0.9.1/lib:$PWD/libs/boost-1.50.0/lib:$PWD/libs/gflags-1.2/lib:$PWD/libs/glog-0.3.3/lib:$PWD/libs/libevent-1.4.10/lib:$PWD/libs/libssl:$PWD/libs/libcrypto
