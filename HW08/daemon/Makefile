NAME=solution
SRC=main.c
OPTS=-Wall -Wextra -Wpedantic -std=c11

all: $(NAME)

$(NAME): $(SRC)
	gcc $(OPTS) $^ -o $@

clean:
	rm -f $(NAME)
