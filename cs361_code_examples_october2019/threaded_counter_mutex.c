#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>

static volatile long     global_counter = 0;
static pthread_mutex_t  counter_lock = PTHREAD_MUTEX_INITIALIZER;

static void*
counter(void *arg)
{
  long   i, limit, local_counter;

  limit = *((long *) arg);
  for (i=0; i<limit; i++)
    {
      pthread_mutex_lock(&counter_lock);
      
      // Start of Critical Section
      local_counter = global_counter;
      ++local_counter;
      global_counter = local_counter;
      // End of Critical Section

      pthread_mutex_unlock(&counter_lock);
    }
  
  return NULL;
}



int
main(int argc, char *argv[])
{
  long       limit;
  pthread_t  thread_a, thread_b;
  
  if (argc <= 1) limit = 100;
  else           limit = atol(argv[1]);

  // Start two threads counting
  pthread_create(&thread_a, NULL, counter, &limit);
  pthread_create(&thread_b, NULL, counter, &limit);

  // Wait for both threads to terminate
  pthread_join(thread_a, NULL);
  pthread_join(thread_b, NULL);

  // Display the result
  printf("Expected: %ld\n", limit*2);
  printf("Actual:   %ld\n", global_counter);
  exit(0);
}
