NAME=solution

# сторонние библиотеки libcurl и libcjson
LIBS=libcurl libcjson

LFLAGS=$(shell pkg-config $(LIBS) --libs  )
CFLAGS=$(shell pkg-config $(LIBS) --cflags)
CC=gcc

all:
	$(CC) -Wall -Wextra -Wpedantic -std=c11 $(CFLAGS) main.c -o $(NAME) $(LFLAGS)

.PHONY: clean

clean:
	rm -f $(NAME) core *.o
