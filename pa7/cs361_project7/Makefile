CC=gcc
CFLAGS=-Wall -pthread
MAKE=make

all: server

server: project7.o sockqueue.o intqueue.o
	$(CC) $(CFLAGS) -o $@ project7.o sockqueue.o intqueue.o -pthread

project7.o: project7.c common.h network.h sockqueue.h intqueue.h
	$(CC) $(CFLAGS) -c $< -o $@

sockqueue.o: sockqueue.c common.h sockqueue.h
	$(CC) $(CFLAGS) -c $< -o $@

intqueue.o: intqueue.c intqueue.h
	$(CC) $(CFLAGS) -c $< -o $@

.PHONY clean:
	@rm *.o server
