CFLAGS=-g -O0 -W -Wall -Werror -std=c11 -DDEBUG=1
LDFLAGS=
DEPS=add_function.o sum_array.o fib_iter.o fib_rec.o find_max.o find_str.o state.o data_processing.o branch.o memory.o conditions.o bits.o
OBJS=armemu.o $(DEPS)
PROG=armemu
CC=clang
AS=as

all: $(PROG)

$(PROG): $(OBJS)

armemu.o : armemu.c 

%.o : %.s
	$(AS) -o $@ $<

.PHONY : clean

clean:
	rm -f $(OBJS) $(PROG)
