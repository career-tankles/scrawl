
FEBIRD_HOME = ../../..

CXXFLAGS := -g3
CXXFLAGS += -Wall -Wextra
CXXFLAGS += -I${FEBIRD_HOME}/include
CXXFLAGS += -I/home/leipeng/opt/include
LDFLAGS = -g3 

CXX = g++
#CXX = env LD_LIBRARY_PATH=${HOME}/opt/lib:${HOME}/opt/lib64 ${HOME}/opt/bin/g++
#-std=gnu++11

.PHONY: all clean

SRC := $(wildcard *.cpp)
BIN_R := $(addprefix rls/, $(basename ${SRC}))
BIN_D := $(addprefix dbg/, $(basename ${SRC}))

ifeq (${STATIC},1)
  LIB_D := -Wl,--whole-archive ${FEBIRD_HOME}/lib/libfebird-d.a -Wl,--no-whole-archive -lz -lbz2 -lrt -pthread
  LIB_R := -Wl,--whole-archive ${FEBIRD_HOME}/lib/libfebird-r.a -Wl,--no-whole-archive -lz -lbz2 -lrt -pthread
else
  LIB_D := -L${FEBIRD_HOME}/lib -lfebird-d
  LIB_R := -L${FEBIRD_HOME}/lib -lfebird-r
endif

all: ${BIN_R} ${BIN_D}

rls/regex_maxmatch_mt : LIB_R += -L${FEBIRD_HOME}/lib -ltbb
dbg/regex_maxmatch_mt : LIB_D += -L${FEBIRD_HOME}/lib -ltbb

rls/% : rls/%.o
	@#echo Release linking ....
	${CXX} -o $@ $^ ${LDFLAGS} ${LIB_R}

dbg/% : dbg/%.o
	@#echo Debug linking ....
	${CXX} -o $@ $^ ${LDFLAGS} ${LIB_D}

rls/%.o : %.cpp
	@#echo Release compiling ...
	@mkdir -p rls
	${CXX} -c -O3 ${CXXFLAGS} -DNDEBUG -o $@ $^

dbg/%.o : %.cpp
	@#echo Debug compiling ...
	@mkdir -p dbg
	${CXX} -c -O0 ${CXXFLAGS} -o $@ $^

clean:
	rm -rf dbg rls

