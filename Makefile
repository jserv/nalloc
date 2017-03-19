CFLAGS = -Wall -g

all:
	$(CC) $(CFLAGS) -c nalloc.c
	$(CC) $(CFLAGS) -c test.c
	$(CC) -o test nalloc.o test.o

clean:
	rm -f nalloc.o test.o test
