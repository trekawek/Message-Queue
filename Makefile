CC=gcc

all:	program

program:	queue.o queue.h program.c
	$(CC) -o program queue.o program.c -pthread

queue.o:	queue.h queue.c
	$(CC) -c queue.c

run:	program
	./program

clean:	
	rm -f *.o program queue.tar.gz

package:	Makefile queue.c queue.h program.c README
	tar -cf queue.tar Makefile queue.c queue.h program.c README && gzip -9 queue.tar
