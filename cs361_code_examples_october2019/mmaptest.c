#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

int main(int argc, char *argv[]) {

  int fd = open(argv[1], O_RDWR);
  int stime = atoi(argv[3]);

  char *faddr = mmap(0, 4096, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
  if (faddr == (void *)-1) {
    printf("Error memory mapping file: %s\n", strerror(errno));
    exit(1);
  }

  for (int i=0; i<20; i++)
    printf("Byte %d: %c\n", i, faddr[i]);

  printf("Sleeping for %d seconds\n", stime);
  sleep(stime);

  printf("Writing %s, starting at Byte %d\n", argv[2], stime);

  memcpy((faddr+stime), argv[2], strlen(argv[2]));
  msync(faddr, 4096, MS_ASYNC);

  printf("Sleeping for %d seconds\n", stime);
  sleep(stime);

  for (int i=0; i<20; i++)
    printf("Byte %d: %c\n", i, faddr[i]);


  munmap(faddr, 4096);
  close(fd);

  return 0;

}
