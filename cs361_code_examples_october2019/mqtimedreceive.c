#include <fcntl.h>
#include <sys/stat.h>
#include <mqueue.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

int main(int argc, char *argv[]) {

  struct mq_attr attr;
  mqd_t mqdes;
  unsigned int prio;
  char *buffer;
  ssize_t n;
  struct timespec timer;

  if ((mqdes = mq_open(argv[1], O_RDONLY | O_CREAT, 0600)) < 0) {
    printf("Error opening message queue %s: %s\n", argv[1], strerror(errno));
    return 1;
  }

  mq_getattr(mqdes, &attr);

  buffer = malloc(attr.mq_msgsize+1);
  if (buffer == NULL)
    return 1;

  // Get current time and add 5 seconds to it
  clock_gettime(CLOCK_REALTIME, &timer);
  timer.tv_sec += 5;

  // Receive the message with timeout
  n = mq_timedreceive(mqdes, buffer, attr.mq_msgsize, &prio, &timer);
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
