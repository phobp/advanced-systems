#include <fcntl.h>
#include <sys/stat.h>
#include <mqueue.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[]) {

  mqd_t mqdes;
  unsigned int prio;

  // Require the name of the queue, a non-NULL message and a 
  // priority
  if (argc != 4) {
    printf("Usage: %s queue_name message priority\n", argv[0]);
    return 0;
  }

  // Open the queue, create if necessary
  if ((mqdes = mq_open(argv[1], O_RDWR | O_CREAT, 0600, NULL)) < 0) {
    printf("Error opening message queue %s: %s\n", argv[1], strerror(errno));
    return 1;
  }

  // Convert third argument to integer
  prio = atoi(argv[3]);

  // Send message to queue
  if (mq_send(mqdes, argv[2], strlen(argv[2]), prio) < 0) {
    printf("Error sending to queue: %s\n", strerror(errno));
    return 1;
  }

  // Close message queue descriptor
  mq_close(mqdes);

  return 0;
}
