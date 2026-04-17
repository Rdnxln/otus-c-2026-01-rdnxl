
all: solution

solution: main.c
	$(CC) $^ -o $@ -Wall -Wextra -Wpedantic -std=c11

clean:
	rm -f solution core
test:
	@[ -x ./solution ] && \
        { \
           md5sum ./*.txt ; \
           ./solution  cp1251.txt      out1  CP-1251    ; \
           ./solution  koi8.txt        out2  KOI8-R     ; \
           ./solution  iso-8859-5.txt  out3  ISO-8859-5 ; \
           md5sum ./out*  ; \
        }

.PHONY: all clean test
