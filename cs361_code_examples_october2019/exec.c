#include <sys/types.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <stdio.h>

int main(int argc, char *argv[]) {

/* Create the first child and run 'ls -l' */
  pid_t child_pid = fork ();

  if (child_pid < 0) exit (1); /* exit if fork() failed */

  if (child_pid == 0){
    int rc = execl ("/bin/ls", "ls", "-l", NULL);
    exit (1); /* exit if exec fails */
  }

  /* Make the parent wait for the first child */
  wait (NULL);

  /* Create another child and re-run 'ls -l' */
  char *const parameters[] = { "ls", "-l", NULL};
  child_pid = fork ();
  if (child_pid < 0) exit (1); /* exit if fork() failed */

  if (child_pid == 0) {
    int rc = execvp ("ls", parameters);
    exit (1); /* exit if exec fails */
  }

  /* Make the parent wait for the second child */
  wait (NULL);

  return 0;
}
