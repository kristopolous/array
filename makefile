CFLAGS=-g -O2
all: array hash
array: array.o parse.o
hash: hash.o parse.o
package:
	tar cjf array.tbz *.c *.h makefile
clean:
	rm -f a.out core array hash *.o
install:
	make
	cp array hash /usr/local/bin
