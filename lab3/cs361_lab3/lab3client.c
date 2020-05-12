/*

    CS 361 Lab 3
    lab3client.c

    Name: Brendan Pho

    Honor Code Statement: This work adheres to the JMU Honor Code.

*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>
#include "network.h"
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <errno.h>

int main(int argc, char *argv[]) {

    uint16_t server_port;
    int message_length;
    int sock;
    struct sockaddr_in address;
    struct dr_short_msg msg;
    unsigned int length;
    const int MAP_ENTRY_LEN = 15;
    
    // check if the amount of arguments is correct
    if (argc != 4) {
        printf("Usage: %s server_ip_address server_port message\n", argv[0]);
        exit(0);
    }

    // get the server port
    server_port = (uint16_t) (strtol(argv[2], NULL, 0) & 0xffff);
    if (server_port == 0) {
        printf("%s: Invalid server port: %s\n", argv[0], argv[2]);
        exit(1);
    }

    // ensure that the message length is 15 characters
    message_length = strlen(argv[3]);
    if (message_length > 15) {
       printf("%s: Message length too big: %d characters. Please limit message to at most 15 characters\n", argv[0], message_length);
       exit(1);
    }

    /* Implement the remaining client code here */

    // create a socket
    sock = socket(AF_INET, SOCK_DGRAM, 0); 
    if (sock < 0) {
        printf("Could not create socket in client: %s\n", strerror(errno));
        exit(1);
    }

    // memset the address to zero
    memset(&address, 0, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_port = htons(server_port);
    address.sin_addr.s_addr = inet_addr(argv[1]);
    
    // memset the msg to zero and set the correct values to send to the server
    memset(&msg, 0, sizeof(msg));
    msg.dr_magic = DR_MAGIC;
    msg.dr_type = MSG_REQUEST;
    strncpy((void *)msg.dr_map_entry, argv[3], MAP_ENTRY_LEN);

    // set the null terminating character for the message
    msg.dr_map_entry[MAP_ENTRY_LEN + 1] = '\0';

    // set the length to the size of the address
    length = sizeof(address);

    // send the message to the server
    if (sendto(sock, &msg, sizeof(struct dr_short_msg), 0, (struct sockaddr *)&address, sizeof(address)) < 0) {
        printf("Could not send in client: %s\n", strerror(errno));
        close(sock);
        exit(1);
    }

    // receive a reply from the server
    if (recvfrom(sock, &msg, sizeof(struct dr_short_msg), 0, (struct sockaddr *)&address, &length) < 0) {
        printf("Could not receive message in client: %s\n", strerror(errno));
        close(sock);
        exit(1);
    }

    // print the message and exit if the magic number and type are correct after receiving message from server
    if (msg.dr_magic == DR_MAGIC && msg.dr_type == MSG_ACCEPT) {
        printf("%s\n", msg.dr_map_entry);
        close(sock);
        exit(0);
    } else {
        printf("Invalid reply\n"); // if magic number and type are not correct, print "Invalid reply"
    }
    
    close(sock);
    return 0;
}

