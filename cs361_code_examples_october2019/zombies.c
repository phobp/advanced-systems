#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
 
#define N 5
 
 
 
int
main(void)
{
  pid_t   pid;
 
  printf("While the parent is sleeping, execute the command:\n");
  printf("ps -l --ppid %ld\n\n", (long)getpid());
 
  
  for (int i=0; i<N; i++)
    {
      pid = fork();
      if (pid == 0) // This is a child process
	{
	  printf("I'm child %d with a PID of %ld\n", i, (long)getpid());
	  exit(0); // Terminate (and become a zombie)
	}
    }
  
  // Executed in the parent only
  sleep(10);
 
  printf("\nAfter the parent terminates, execute the command again:\n");
  printf("ps -l --ppid %ld\n\n", (long)getpid());
 
  exit(0);
}
