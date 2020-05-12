#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>

int main(int argc, char *argv[]) {

  pid_t child_pid = fork ();

  if (child_pid == 0)
    printf ("Hi, I am the child! My process ID is %d. My parent is %d.\n",
              getpid(), getppid());
  else
    printf ("Parent %d just learned about child %d\n", getpid(), child_pid);

  child_pid = fork ();
  if (child_pid == 0)
    printf ("Hi, I am the child! My process ID is %d. My parent is %d.\n",
              getpid(), getppid());
  else
    printf ("Parent %d just learned about child %d\n", getpid(), child_pid);

  return 0;
}
