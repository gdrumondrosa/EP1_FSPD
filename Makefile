CC = gcc

.PHONY: clean build

build: ex1

ex1: ex1.c
	$(CC) ex1.c -o ex1

clean:
	rm -f ex1