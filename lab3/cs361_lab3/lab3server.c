/*

    CS 361 Lab 3
    lab3server.c

    Name: Brendan Pho

    Honor Code Statement: This work adheres to the JMU Honor Code.

*/

#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <string.h>
#include "network.h"
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <errno.h>

int main(int argc, char *argv[]) {

    uint16_t req_listen_port;           // port number to listen for requests
    int sock;
    unsigned int length;
    struct sockaddr_in address;
    struct dr_short_msg msg;
    const int MAP_ENTRY_LEN = 15;
    
    // check if the amount of arguments is correct
    if (argc != 2) {
        printf("Usage: %s listen_port\n", argv[0]);
        exit(0);
    }

    /* Try to convert argv[1] to requests port number */
    req_listen_port = (uint16_t) (strtol(argv[1], NULL, 0) & 0xffff);
    if (req_listen_port == 0) {
        printf("Invalid requests port: %s\n", argv[1]);
        exit(1);
    }


    /* Implement the remaining server code here */
    sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) {
        printf("Could not create socket in server: %s\n", strerror(errno));
        exit(1);
    }

    // memset the address to zero
    memset(&address, 0, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_port = htons(req_listen_port);
    address.sin_addr.s_addr = htonl(INADDR_ANY);

    // bind the socket to the address
    if (bind(sock, (struct sockaddr *)&address, sizeof(address)) < 0) {
        printf("Could not bind in server: %s\n", strerror(errno));
        exit(1);
    }

    // continuously receive messages until you receive a type of MSG_FINISHED
    while (1) {

        if(recvfrom(sock, &msg, sizeof(struct dr_short_msg), 0, (struct sockaddr *)&address, &length) < 0) {
            printf("Could not receive message in server: %s\n", strerror(errno));
            close(sock);
            exit(1);
        }

        // set the null terminating character for the message string
        msg.dr_map_entry[MAP_ENTRY_LEN] = '\0';

        // if the magic number and type are correct, set the type to MSG_ACCEPT and send the message back to client
        if (msg.dr_magic == DR_MAGIC && msg.dr_type == MSG_REQUEST) {
            msg.dr_type = MSG_ACCEPT;
            if(sendto(sock, &msg, sizeof(struct dr_short_msg), 0, (struct sockaddr *)&address, sizeof(address)) < 0) {
                printf("Could not send in server: %s\n", strerror(errno));
                close(sock);
                exit(1);
            }
        }
        
        // exit infinite loop when type is MSG_FINISHED
        if (msg.dr_type == MSG_FINISHED) {
           close(sock);
           exit(0); 
        }     
    }

    close(sock);
    return 0;
}
