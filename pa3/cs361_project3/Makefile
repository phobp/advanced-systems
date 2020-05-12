CC=gcc
CFLAGS=-Wall
MAKE=make

all: mapper

mapper: project3.o classify.o
	$(CC) $(CFLAGS) -o $@ project3.o classify.o -lm

project3.o: project3v2.c classify.h common.h
	$(CC) $(CFLAGS) -c $< -o $@

classify.o: classify.c classify.h common.h
	$(CC) $(CFLAGS) -c $< -o $@

.PHONY clean:
	@rm *.o mapper
