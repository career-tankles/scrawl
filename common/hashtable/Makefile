CC	:= g++
CPP	:= g++
AR	:= ar cr
RM	:= /bin/rm -f
CP	:= /bin/cp -i


INCS	:= include/
LIBS	:=
CFLAGS	:= -g -Wall -fPIC

BIN		:= hashtable-test
SRCS    := $(wildcard *.c) 
CPPSRCS := $(wildcard *.cpp) 
OBJS	:= $(SRCS:%.c=%.o ) $(CPPSRCS:%.cpp=%.o)
INCLS	:= $(INCS:%=-I%)

.PHONY: all clean
all: $(BIN)

$(BIN): $(OBJS)

clean:
	-$(RM) $(BIN) $(OBJS)

.c.o:
	$(CC) -c $(INCLS) -o $@ $< $(CFLAGS)

.cpp.o:
	$(CC) -c $(INCLS) -o $@ $< $(CFLAGS)
