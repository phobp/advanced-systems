CC=gcc
CFLAGS=-Wall
MAKE=make

all: lab3server lab3client

lab3server: lab3server.o
	$(CC) $(CFLAGS) -o $@ $<

lab3client: lab3client.o
	$(CC) $(CFLAGS) -o $@ $<

lab3server.o: lab3server.c network.h
	$(CC) $(CFLAGS) -c $< -o $@

lab3client.o: lab3client.c network.h
	$(CC) $(CFLAGS) -c $< -o $@


.PHONY clean:
	@rm -f *.o lab3server lab3client
