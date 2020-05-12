#include <fcntl.h>
#include <sys/stat.h>
#include <mqueue.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[]) {

  struct mq_attr attr;
  mqd_t mqdes;
  unsigned int prio;
  char *buffer;
  ssize_t n;

  // Open the queue, create if necessary
  if ((mqdes = mq_open(argv[1], O_RDONLY)) < 0) {
    printf("Error opening message queue %s: %s\n", argv[1], strerror(errno));
    return 1;
  }

  // Get attributes for the queue
  mq_getattr(mqdes, &attr);

  // Allocate large enough buffer
  buffer = malloc(attr.mq_msgsize+1);
  if (buffer == NULL)
    return 1;

  // Receive the message
  n = mq_receive(mqdes, buffer, attr.mq_msgsize, &prio);
  if (n < 0) {
    printf("Error receiving message from %s: %s\n", argv[1], strerror(errno));
    return 1;
  }
  buffer[n] = '\0';
  printf("Size: %ld, Prio: %d, Message: %s\n", (long)n, prio, buffer);

  free(buffer);
  mq_close(mqdes);

  return 0;
}
