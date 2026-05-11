# LERR is
# (L)ogger and
# (E)(R)(R)ors

NAME    =lerr
LIBNAME =lib$(NAME)
OPTS    =-Wall -Wextra -Wpedantic -std=c11

CFLAGS  =
LDLIBS  =

.PHONY: all clean $(NAME).c $(NAME).h

all:	$(LIBNAME).a

$(LIBNAME).a: $(NAME).o
	ar  cr  $@  $^

$(NAME).o: $(NAME).h
	gcc $(OPTS) -g -c $(NAME).c -o $@

clean:
	rm -f $(NAME).o $(LIBNAME).a $(LIBNAME).so

test:
	pushd ..; ./solution_run.sh ; popd
