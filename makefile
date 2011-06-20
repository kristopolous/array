CFLAGS=-g -O2
all: array hash
array: array.o parse.o
hash: hash.o parse.o
package:
	cd .. && tar cjf array/array-latest.tbz array/*.c array/*.h array/makefile
clean:
	rm -f a.out core array hash *.o
install:
	make
	cp array hash /usr/local/bin
