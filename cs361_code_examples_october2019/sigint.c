#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>

static void
handler(int sig) {
      write(1, "HaHa\n", 5);
}

int main(void) {
// Setup the signal handler
  signal(SIGINT, handler);

  while(1);

  return 0;
}
