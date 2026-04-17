
all: solution

solution: main.c
	$(CC) $^ -o $@ -Wall -Wextra -Wpedantic -std=c11

clean:
	rm -f solution core

test:
	@[ -x ./solution ] && ./solution *.{jpg,png}

.PHONY: all clean test
