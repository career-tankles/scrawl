CC	:= gcc
AR	:= ar cr
RM	:= /bin/rm -f
CP	:= /bin/cp -i


INCS	:=
LIBS	:=
CFLAGS	:= -g -Wall -fPIC

BIN		:= datrie_tool
SRCS    := $(wildcard *.c)
OBJS	:= $(SRCS:%.c=%.o)

.PHONY: all clean
all: $(BIN)

$(BIN): $(OBJS)

clean:
	-$(RM) $(BIN) $(OBJS)

.c.o:
	$(CC) -c -o $@ $< $(CFLAGS)

