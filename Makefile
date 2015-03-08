CC=gcc
CFLAGS=-g -Wall
OBJS=ops.c vector.c parse.c asm.c


all: ops.h vector.h parse.h $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o $(AS)


clean:
	rm -f as *.o *~
