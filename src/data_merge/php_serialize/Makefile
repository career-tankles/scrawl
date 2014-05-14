# set CPUS for Linux or FreeBSD
PLATFORM := $(shell uname)
PLATFORM_VERSION := $(shell uname -r| awk -F 'el' '{printf("el%d", substr($$2,1,1))}')
CPUS := $(strip $(if $(shell echo $(PLATFORM)|grep Linux),\
	$(shell cat /proc/cpuinfo|grep -c processor),\
	$(shell sysctl -a | egrep -i 'hw.ncpu' | cut -d: -f2)))

CXX11_HOME = /usr/local/gcc-4.7.2
CXX := ${CXX11_HOME}/bin/g++

LD_LIBRARY_PATH=${CXX11_HOME}/lib:${CXX11_HOME}/lib64:$(DEFAULT_LIB_INSTALL_PATH)

CXXFLAGS := -g3 -O2 -fno-strict-aliasing -Wall -Werror -fPIC \
	-I$(HOME)/qmodule/CloudLog/include \
	-I$(HOME)/qmodule/febird/include \
	-I$(HOME)/qmodule/febird-0.1/include \
	-I/usr/include/boost141 \
	-I../../../libs/boost-1.50.0/include

LDFLAGS := -pthread \
			-L /home/s/lib \
			-L/usr/lib64/boost141 \
			-L../../../libs/boost-1.50.0/lib

RTFLAGS := -Wl,-R/home/s/lib
LIBS := 

SRC := $(wildcard *.cpp)
OBJ := $(patsubst %.cpp, %.o, $(SRC))
DEP := $(patsubst %.o, %.d, $(OBJ))

TARGET := libutil_php_serialize.a

ifeq ($(USE_DEP),1)
-include $(DEP)
endif

all: 
	#$(MAKE) -j$(CPUS) USE_DEP=1 target
	$(MAKE)  USE_DEP=1 target
	$(MAKE)  php_test

libutil_php_serialize.a: $(OBJ)
	ar rcs $@ $^

php_test: php_test.cc libutil_php_serialize.a
	$(CXX) -D__TEST_PHP $(CXXFLAGS) -o $@ $^

target: $(TARGET)

%.o : %.cc
	$(CXX) -c $(CXXFLAGS) $< -o $@
%.o : %.cpp
	$(CXX) -c $(CXXFLAGS) $< -o $@

%.d : %.cc
	$(CXX) -MM $< $(CXXFLAGS) | sed 's/$(notdir $*)\.o/$(subst /,\/,$*).o $(subst /,\/,$*).d/g' > $@
%.d : %.cpp
	$(CXX) -MM $< $(CXXFLAGS) | sed 's/$(notdir $*)\.o/$(subst /,\/,$*).o $(subst /,\/,$*).d/g' > $@

clean:
	-rm -rf $(OBJ) $(TARGET) *.pid *.log *.core $(DEP) *.so *.so.* php_test php_test.d
	#$(MAKE) clean -C test

.PHONY: all target clean target install
