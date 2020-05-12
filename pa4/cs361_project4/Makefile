CC=gcc
CFLAGS=-Wall
MAKE=make

all: classifier-pipes

classifier-pipes: project4.o classify.o
	$(CC) $(CFLAGS) -o $@ project4.o classify.o -lm


project4.o: project4.c classify.h common.h
	$(CC) $(CFLAGS) -c $< -o $@

classify.o: classify.c classify.h common.h
	$(CC) $(CFLAGS) -c $< -o $@

.PHONY clean:
	@rm *.o classifier-pipes
