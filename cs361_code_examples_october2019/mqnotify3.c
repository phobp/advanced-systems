#include <fcntl.h>
#include <sys/stat.h>
#include <mqueue.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>

volatile sig_atomic_t mqflag = 0;
static void sig_usr1(int);

int main(int argc, char *argv[]) {

  struct mq_attr attr;
  mqd_t mqdes;
  char *buffer;
  struct sigevent sigev;
  ssize_t n;
  sigset_t newmask;

  if ((mqdes = mq_open(argv[1], O_RDONLY | O_CREAT | O_NONBLOCK, 0600, NULL)) < 0) {
    printf("Error opening message queue %s: %s\n", argv[1], strerror(errno));
    return 1;
  }
  mq_getattr(mqdes, &attr);
  buffer = malloc(attr.mq_msgsize+1);
  if (buffer == NULL)
    return 1;

  signal(SIGUSR1, sig_usr1); // register signal handler for SIGUSR1
  sigev.sigev_notify = SIGEV_SIGNAL; // tell mq_notify() to send a signal
  sigev.sigev_signo = SIGUSR1; // tell mq_notify() to send SIGUSR1
  if (mq_notify(mqdes, &sigev) < 0) {
    printf("Error registering signal notification: %s\n", strerror(errno));
    return 1;
  }

  sigemptyset(&newmask);
  sigaddset(&newmask, SIGUSR1);

  while(1) {

    while(mqflag == 0) {
      // do stuff
    }
    mqflag = 0;
    mq_notify(mqdes, &sigev); // re-register to be notified
    sigprocmask(SIG_BLOCK, &newmask, NULL);
    while((n = mq_receive(mqdes, buffer, attr.mq_msgsize, NULL)) >= 0) {
      buffer[n] = '\0';
      printf("Size: %ld, Message: %s\n", (long)n, buffer);
    }
    if (errno != EAGAIN) {
      printf("Error receiving message: %s\n", strerror(errno));
      exit(1);
    }
    sigprocmask(SIG_UNBLOCK, &newmask, NULL);
  }

  free(buffer);
  mq_close(mqdes);

  return 0;
}

static void sig_usr1(int signo) {
  mqflag = 1;
}
