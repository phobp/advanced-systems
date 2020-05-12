/*
    Name: Brendan Pho
    This work adheres to the JMU honor code.
*/

#include <fcntl.h>
#include <sys/stat.h>
#include <mqueue.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>

struct msg {
    char magic[8];
    uint32_t id;
    char now[200];
    char data[36];
};

int main(int argc, char *argv[]) {
    
    mqd_t mq_descriptor; 
    struct mq_attr attr;
    struct msg *message;
    char *buffer;
    char magic_buffer[9];
    unsigned int prio;
    ssize_t n;
    char magic_str[] = "cs361lab\0";
    
    if (argc != 2) {
        printf("Must give one POSIX message queue for reading.\n");
        return 1;
    }

    if ((mq_descriptor = mq_open(argv[1], O_RDONLY, 0600, &attr)) < 0) {
        printf("Error opening message queue %s: %s\n", argv[1], strerror(errno));
        return 1;
    }
    
    
    if ((mq_getattr(mq_descriptor, &attr)) < 0) {
        printf("Error getting message attributes for %s: %s\n", argv[1], strerror(errno));
        return 1;
    }
 
    buffer = calloc(attr.mq_msgsize + 1, 1);
    if (buffer == NULL) {
        return 1;
    }
    
    n = mq_receive(mq_descriptor, buffer, attr.mq_msgsize, &prio); 
    if(n < 0) {
        printf("Error receiving message from %s: %s\n", argv[1], strerror(errno));
        return 1;
    }
    buffer[n] = '\0';
    
    
    printf("Queue %s:\n", argv[1]);
    printf("\tFlags:\t\t\t%ld\n", attr.mq_flags);
    printf("\tMax messages:\t\t%ld\n", attr.mq_maxmsg);
    printf("\tMax size:\t\t%ld\n", attr.mq_msgsize);
    printf("\tCurrent messages:\t%ld\n", attr.mq_curmsgs);

    message = (struct msg *)buffer;

    magic_buffer[8] = '\0';
    strncpy(magic_buffer, message->magic, 8);

    if (strncmp(magic_str, message->magic, sizeof(strlen(magic_str))) == 0) {
        
        printf("\n\nReceived message (%ld bytes):\n", sizeof(struct msg));
        printf("\tMagic: \t\t%s\n", magic_buffer);
        printf("\tID: \t\t%u\n", message->id);
        printf("\tTime: \t\t%s\n", message->now);
        printf("\tData: \t\t%s\n", message->data);
    } else {
       
        printf("\nReceived message (%ld bytes):\n", sizeof(struct msg));
        printf("\tInvalid message\n"); 
    }
    
    free(buffer);
    mq_close(mq_descriptor);

    return 0;
}
