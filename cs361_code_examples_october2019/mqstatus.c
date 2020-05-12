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

  printf("Message queue %s:\n\tFlags:\t%d\n\tMaxmsg:\t%d\n\tMaxsz:\t%d\n\tCurmsg:\t%d\n", argv[1], attr.mq_flags, attr.mq_maxmsg, attr.mq_msgsize, attr.mq_curmsgs);

  mq_close(mqdes);

  return 0;
}
