AS = nasm  # NOTE: yasm тоже подойдёт

all: asm-prog asm-prog2 c-prog c-prog2

asm-prog: main.o
	$(CC) -g -no-pie $^ -o $@

asm-prog2: main_bonus2.o
	$(CC) -g -no-pie $^ -o $@

main_bonus2.o: main_bonus2.asm
	$(AS) -g -felf64 $^

main.o: main.asm
	$(AS) -g -felf64 $^

c-prog: reverse_main.c
	$(CC) -Wall -Wextra -Wpedantic -std=c11 $^ -o $@
	$(CC) -S -masm=intel -fno-asynchronous-unwind-tables -fno-dwarf2-cfi-asm $^

c-prog2: reverse_main_no_recursive.c
	$(CC) -Wall -Wextra -Wpedantic -std=c11 $^ -o $@
	$(CC) -S -masm=intel -fno-asynchronous-unwind-tables -fno-dwarf2-cfi-asm $^

clean:
	rm -f asm-prog *.o core c-prog c-prog2 reverse_main.s reverse_main_no_recursive.s

.PHONY: clean
