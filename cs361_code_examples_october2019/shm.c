#include <sys/mman.h>
#include <assert.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>

struct permission {
  int user;
  int group;
  int other;
};

int main(int argc, char *argv[]) {

  int shmfd = shm_open ("/OpenCSF_SHM", O_CREAT | O_EXCL | O_RDWR,
                      S_IRUSR | S_IWUSR);
  assert (shmfd != -1);

  /* Resize the region to store 1 struct instance */
  assert (ftruncate (shmfd, sizeof (struct permission)) != -1);

  /* Map the object into memory so file operations aren't needed */
  struct permission *perm =
    mmap (NULL, sizeof (struct permission), PROT_READ | PROT_WRITE,
        MAP_SHARED, shmfd, 0);
  assert (perm != MAP_FAILED);

  /* Create a child process and write to the mapped/shared region */
  pid_t child_pid = fork();
  if (child_pid == 0)
  {
    perm->user = 6;
    perm->group = 4;
    perm->other = 0;

    /* Unmap and close the child's shared memory access */
    munmap (perm, sizeof (struct permission));
    close (shmfd);
    return 0;
  }

  /* Make the parent wait until the child has exited */
  wait (NULL);

  /* Read from the mapped/shared memory region */
  printf ("Permission bit-mask: 0%d%d%d\n", perm->user, perm->group, 
        perm->other);

  sleep(10);

  /* Unmap, close, and delete the shared memory object */
  munmap (perm, sizeof (struct permission));
  close (shmfd);
  shm_unlink ("/OpenCSF_SHM");

}
