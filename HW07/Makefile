LOGFILE=./_logs.txt

SRCS = \
  danger_code.c \
  log_spammer.c


OBJS =$(SRCS:.c=.o)
HDRS =$(SRCS:.c=.h)

OPTS =-Wall -Wextra -Wpedantic -std=c11

# имитация флагов из pkg-config
LERR_LDLIBS=-rdynamic -L./lerr -llerr
LERR_CFLAGS=-I./lerr

all:    lib solution

lib:
	pushd ./lerr 1>/dev/null 2>/dev/null; make; popd 1>/dev/null 2>/dev/null

solution: main.c $(OBJS)
	gcc -g $(OPTS) $(LERR_CFLAGS) ./main.c -o ./solution $(OBJS)  $(LERR_LDLIBS)

$(OBJS):    $(SRCS)
	gcc $(OPTS) -I./lerr -c $^

.PHONY: clean

clean:
	rm -f ./solution $(LOGFILE) $(OBJS)
