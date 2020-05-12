#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>

int main(int argc, char *argv[]) {

  int fd = open(argv[1], O_RDONLY);
  char buffer[5];
  buffer[4] = '\0';

  pid_t child_pid = fork ();

  if (child_pid == 0) {
    read(fd, buffer, 4);
sleep(1);
    printf ("Child read:\t%s\n", buffer);
  }
  else {
sleep(1);
    read(fd, buffer, 4);
    printf ("Parent read:\t%s\n", buffer);
  }

  return 0;
}
