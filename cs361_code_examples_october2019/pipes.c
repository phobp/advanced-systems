#include <unistd.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

int main(int argc, char *argv[]) {

  int pipefd[2];
  char buffer[10];
  /* Clear the buffer */
  memset (buffer, 0, sizeof (buffer));

  /* Open the pipe */
  if (pipe (pipefd) < 0)
  {
    printf ("ERROR: Failed to open pipe\n");
    exit (1);
  }

  /* Create a child process */
  pid_t child_pid = fork ();
  assert (child_pid >= 0);
  if (child_pid == 0)
  {
    /* Child closes write-end, then reads from the pipe */
    close (pipefd[1]);
    ssize_t bytes_read = read (pipefd[0], buffer, 10);
    if (bytes_read <= 0) exit (0);

    printf ("Child received: '%s'\n", buffer);
    exit (0);
  }

  /* Parent closes the unused reading end */
  close (pipefd[0]);

  /* Parent sends 'hello' and waits */
  strncpy (buffer, "hello", sizeof (buffer));
  printf ("Parent is sending '%s'\n", buffer);
  write (pipefd[1], buffer, sizeof (buffer));
  wait (NULL);
  printf ("Child should have printed the message\n");

  return 0;
}
