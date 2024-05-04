CC = gcc
CFLAGS = -Wall -fsanitize=address -std=c99 -O2

spchk: spchk.o
	$(CC) $(CFLAGS) spchk.o -o spchk

spchk.o: spchk.c
	$(CC) $(CFLAGS) -c spchk.c

clean:
	rm -f *.o spchk errors