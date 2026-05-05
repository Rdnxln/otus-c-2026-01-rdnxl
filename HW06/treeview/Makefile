NAME=solution
SRC=main.c
OPTS=-Wall -Wextra -Wpedantic -std=c11

all: $(NAME)

$(NAME): $(SRC)
	gcc $(OPTS) $^ -o $@ $(shell pkg-config gtk+-3.0 --cflags --libs)

.PHONY: clean all

clean:
	rm -f $(NAME)
