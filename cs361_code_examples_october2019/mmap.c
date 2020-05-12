
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <assert.h>
#include <stdio.h>

int main(int argc, char *argv[]) {

  int fd = open("/bin/bash", O_RDONLY);
  assert (fd != -1);

  struct stat file_info;
  assert (fstat(fd, &file_info) != -1);

  char *mmap_addr = mmap(NULL, file_info.st_size, PROT_READ | PROT_WRITE, 
                         MAP_SHARED, fd, 0);
  assert (mmap_addr != MAP_FAILED);

  printf("%c%c%c\n", mmap_addr[1], mmap_addr[2], mmap_addr[3]);

  munmap(mmap_addr, file_info.st_size);

  close(fd);

  return 0;
}
