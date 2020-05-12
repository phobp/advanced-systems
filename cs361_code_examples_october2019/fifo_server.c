#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <assert.h>

int main(int argc, char* argv[]) {

  const char *FIFO = "/tmp/MY_FIFO";
  assert (mkfifo (FIFO, S_IRUSR | S_IWUSR) == 0);

  /* Try to open the FIFO. Delete FIFO if open() fails */
  int fifo = open (FIFO, O_RDONLY);
  if (fifo == -1) {
    fprintf (stderr, "Failed to open FIFO\n");
    unlink (FIFO);
    return 1;
  }

  /* Main server loop */
  while (1) {
    int req = 0;
    if (read (fifo, &req, sizeof (int)) != sizeof (int))
      continue;

    /* If we read a 0, quit; otherwise print hello */
    if (req == 0) break;
    printf ("hello\n");
  }

  /* Read a 0 from the FIFO, so close and delete the FIFO */
  close (fifo);
  printf ("Deleting FIFO\n");
  unlink (FIFO);



  return 0;
}
