CC=gcc
CFLAGS=-Wall

all: mapper-with-queues

mapper-with-queues: project5.o classify.o intqueue.o
	$(CC) $(CFLAGS) -o $@ project5.o classify.o intqueue.o -lm -lrt

pa5.o: project5.c classify.h common.h intqueue.h
	$(CC) $(CFLAGS) -c $< -o $@

classify.o: classify.c classify.h common.h
	$(CC) $(CFLAGS) -c $< -o $@

intqueue.o: intqueue.c intqueue.h
	$(CC) $(CFLAGS) -c $< -o $@

.PHONY clean:
	@rm -f *.o mapper-with-queues /dev/mqueue/tasks_phobp /dev/mqueue/results_phobp classification map
