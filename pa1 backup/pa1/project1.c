/*
	Project 1
	Name: Brendan Pho
	This work adheres to the JMU honor code
*/

#include <stdint.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

int main(int argc, char *argv[]) {

    char fn[12];
    uint32_t cluster_num;
    int i = 0;
    const uint32_t CLUSTER_SIZE = 4096;
    char buf[4096];

    // Checks correct number of arguments
    if (argc < 3) {
        printf("Must give an input file and map file.\n");
        exit(0);
    }

    // Checks correct number of arguments
    if (argc > 3) {
        printf("Only give 2 arguments.\n");
        exit(0);
    }

    // Opens the input file
    int fdinput = open(argv[1], O_RDONLY);
    if (fdinput == -1) {
        printf("Error opening input file. errno %d\n", fdinput);
        exit(0);
    }

    // Opens the map file
    int fdmap = open(argv[2], O_RDONLY);
    if (fdmap == -1) {
        printf("Error opening map file. errno %d\n", fdmap);
        exit(0);
    }

    //Iterates through the map file
    while (read(fdmap, &fn, sizeof(char) * 12) > 0 && read(fdmap, &cluster_num, sizeof(cluster_num) > 0))
    {
        int fdx = open(fn, O_RDWR | O_CREAT, 0700);
        lseek(fdx, cluster_num * CLUSTER_SIZE, SEEK_SET);
        lseek(fdinput, i * CLUSTER_SIZE, SEEK_SET);
        read(fdinput, buf, CLUSTER_SIZE);
        write(fdx, buf, 4096);
        close(fdx);
        i += 1;
    }
}
