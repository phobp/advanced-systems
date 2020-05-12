/* CS 361 project7.c

  Name: Brendan Pho
  Honor Code Statement: This work adheres to the JMU honor code.

*/

#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <semaphore.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <arpa/inet.h>

#include "common.h"
#include "network.h"
#include "intqueue.h"
#include "sockqueue.h"

uint16_t req_listen_port;	// the port number to listen on for map and join requests
uint16_t worker_listen_port;	// the port number to listen on for results
sem_t sem_classification;	// used to signal that the classification file has been transferred
sem_t sem_map;			// used to signal that the map file has been generated
sem_t sem_worker;		// used to signal that the first worker has joined
ssize_t class_file_size;	// shared between main thread and transfer thread
struct sockqueue workerq;	// queue of workers ready to perform tasks
pthread_mutex_t wq_mutex = PTHREAD_MUTEX_INITIALIZER; // mutex to protect  workerq
pthread_cond_t wq_cond = PTHREAD_COND_INITIALIZER;  // used to signal when workerq becomes non-empty
struct sock_busy_list busy_workers; // list of workers that are currently performing map tasks
pthread_mutex_t busy_mutex = PTHREAD_MUTEX_INITIALIZER; // mutex to protect busy_workers
pthread_cond_t busy_cond = PTHREAD_COND_INITIALIZER;  // used to signal when busy list becomes empty
int map_fd;			// shared between transfer task and results task
				// there will not be concurrent operations, though, so no mutex is necessary
int busy = 1;			// shared between main thread and transfer thread

void *do_transfers(void *arg) {

    int listen_socket, connection_socket; // two TCP sockets, one for listening and one for established connection
    struct sockaddr_in listen_addr;	// listenin address for the TCP file transfer
    int classification_fd;
    ssize_t data_read;
    char buffer[4096];			// size of data chunks to transmit map file 
    char *class_buffer;			// buffer to receive classification file
					// this will be read in all at once, so buffer
                                        // needs to be allocated dynamically

    /* Setup to listen on all interfaces and the same port as the requests for UDP */
    memset(&listen_addr, 0, sizeof(listen_addr));
    listen_addr.sin_family = AF_INET;
    listen_addr.sin_port = htons(req_listen_port);
    listen_addr.sin_addr.s_addr = INADDR_ANY;

    listen_socket = socket(AF_INET, SOCK_STREAM, 0); // create TCP socket

    /* If address is already in use, this might fail */
    if (bind(listen_socket, (struct sockaddr *)&listen_addr, sizeof(listen_addr)) < 0) {
        printf("Error binding TCP request port: %s\n", strerror(errno));
        exit(1);
    }
    listen(listen_socket, 0);

    /* Forever wait for incoming connections */
    while(1) {
        /* Wait for a requester to connect to us */
        connection_socket = accept(listen_socket, NULL, NULL);

        /* Create new empty classification file */
        classification_fd = open(CLASSIFICATION_FILE, O_WRONLY | O_CREAT | O_TRUNC, 0600);
        if (classification_fd < 0) {
            printf("Error creating classification file: %s\n", strerror(errno));
            exit(1);
        }
        /* Allocate storage for the classification file to be received via TCP.
           The main thread will have set class_file_size. */
        class_buffer = (char *)malloc(class_file_size);

        /* Read the classification file data via the TCP socket */
        if (read(connection_socket, class_buffer, class_file_size) < 0) {
            printf("Error receiving classification file: %s\n", strerror(errno));
            exit(1);
        }

        /* Write the data to the classification file */
        write(classification_fd, class_buffer, class_file_size);

        /* Release resources */
        close(classification_fd);
        free(class_buffer);

        /* Create empty map file */
        map_fd = open(MAP_FILE, O_RDWR | O_CREAT | O_TRUNC, 0600);
        if (map_fd < 0) {
            printf("Error creating map file: %s\n", strerror(errno));
            exit(1);
        }
        /* Signal to the task thread that the classification file has arrived */
        sem_post(&sem_classification);

        /* Wait until the map file has been completed */
        sem_wait(&sem_map);

        /* Seek to the start of the map file */
        lseek(map_fd, 0, SEEK_SET);

        /* Read data in 4096-byte chunks from the map file, and send them
           via the TCP socket to the requester */
        while(0 < (data_read = read(map_fd, &buffer, sizeof(buffer)))) {
            write(connection_socket, &buffer, data_read);
        }

        /* Close TCP connection with requester */
        close(connection_socket);

        /* Close map file */
        close(map_fd);

        /* Delete map and classification files */
        unlink(MAP_FILE);
        unlink(CLASSIFICATION_FILE);

        /* The server is ready to accept another request now */
        busy = 0;
    }

}

void *do_tasks(void *arg) {

    int worker_socket;		// UDP socket used to send MSG_MAPs to workers
    struct dr_long_msg map_task; // Long struct for MSG_MAP messages
    struct intqueue headerq;	// used to store cluster numbers that have a header
    int file_counter;		// used for file name generation
    int classification_fd;	// file descriptor for classification file
    unsigned char classification; // used to read the classification data for one cluster
    int current_cluster;	// used to keep track of which cluster is examined for a header
    struct sockaddr_in *next_worker; // used to get the next worker from the available workers queue
    // create UDP socket
    worker_socket = socket(AF_INET, SOCK_DGRAM, 0);
    if (worker_socket < 0) {
        printf("Could not create socket in tasks: %s\n", strerror(errno));
        exit(1);
    }

    // initialize header queue
    initqueue(&headerq);

    // initialize busy worker list
    sock_busy_init(&busy_workers);
    sem_wait(&sem_worker); // wait until there is at least one worker
    /* Wait forever for classification files to arrive, process them, and assign map tasks */
    while(1) {
        sem_wait(&sem_classification); // wait until the next classification file has arrived
        // reset the file counter
        file_counter = 1;

        // reset current cluster
        current_cluster = -1;

        // open the classification file for reading
        classification_fd = open(CLASSIFICATION_FILE, O_RDONLY | O_CREAT, 0600);
        if (classification_fd < 0) {
            printf("Error creating classification file: %s\n", strerror(errno));
            close(worker_socket);
            exit(1);
        }

        /* scan the classification file for headers and add them to the header queue */
        while (read(classification_fd, &classification, sizeof(unsigned char))) {
            current_cluster++;
            if (classification & TYPE_JPG_HEADER || classification & TYPE_HTML_HEADER) {
                enqueue(&headerq, current_cluster);
            }
        }

        /* generate a map message for each header, and send it off to an available worker */
        while (!isempty(&headerq)) {

            // zero out map_task
            memset(&map_task, 0, sizeof(struct dr_long_msg));

            /* Populate map_task fields */
            map_task.dr_magic = DR_MAGIC;
            map_task.dr_number = dequeue(&headerq);
            map_task.dr_type = MSG_MAP;
            lseek(classification_fd, map_task.dr_number, SEEK_SET);
            read(classification_fd, &classification, sizeof(unsigned char));
            if (classification & TYPE_JPG_HEADER) {
                sprintf(map_task.dr_filename, "file%04d.jpg", file_counter++);
            } else {
                sprintf(map_task.dr_filename, "file%04d.htm", file_counter++);
            }

            /* Copy the chunk of the classification file that starts with the current
               header cluster */
            lseek(classification_fd, map_task.dr_number, SEEK_SET);
            int read_value = read(classification_fd, map_task.dr_data, DR_CLASSIFICATION_CHUNK);
            if(read_value < 0) {
                printf("Failed to read in tasks\n");
                close(worker_socket);
                close(classification_fd);
                exit(1);
            }
            /* Check if available worker queue is not empty. If it is, wait on wq_cond */
            pthread_mutex_lock(&wq_mutex);
            while (sock_isempty(&workerq)) {
                pthread_cond_wait(&wq_cond, &wq_mutex);
            }

            /* Get next available worker from worker queue */
            next_worker = sock_dequeue(&workerq);
            pthread_mutex_unlock(&wq_mutex);

            /* Add worker to busy list */
            pthread_mutex_lock(&busy_mutex);
            sock_busy_add(&busy_workers, next_worker);
            pthread_mutex_unlock(&busy_mutex);

            /* Send message to worker */
            if (sendto(worker_socket, &map_task, sizeof(struct dr_long_msg), 0, (struct sockaddr *)next_worker, sizeof(*next_worker)) < 0) {
                printf("Could not send in tasks: %s\n", strerror(errno));
                close(worker_socket);
                close(classification_fd);
                exit(1);
            }
        }

        /* Check if busy worker list is empty. If it is not, then loop waiting on busy_cond
           until it is */
        pthread_mutex_lock(&busy_mutex);
        while(!sock_busy_isempty(&busy_workers)) {
            pthread_cond_wait(&busy_cond, &busy_mutex);
        }
        pthread_mutex_unlock(&busy_mutex);

        /* No more workers in the busy list, and no more clusters in the header queue: all
           mapping is completed. Signal to transfer thread that map file is ready */
        sem_post(&sem_map);
        
    }
    
    /* Free resources */
    close(worker_socket);
    close(classification_fd);
}

void *do_results(void *arg) {

    int worker_socket;			// UDP socket on which to receive map results
    struct sockaddr_in worker_address;  // Address to listen on worker port
    struct sockaddr_in peer_address;    // Address of the worker that sent us a result
    struct sockaddr_in *finished_worker;  // Worker to be removed from nusy list
    socklen_t addr_len;			// used for recvfrom calls
    struct dr_short_msg result;		// buffer for result messages

    /* Create UDP socket */
    worker_socket = socket(AF_INET, SOCK_DGRAM, 0);
    if (worker_socket < 0) {
        printf("Could not create socket in worker: %s\n", strerror(errno));
        exit(1);
    }
    
    /* Zero out worker address and configure it to listen on all available interfaces on
       the worker listen port */
    memset(&worker_address, 0, sizeof(worker_address));
    worker_address.sin_family = AF_INET;
    worker_address.sin_port = htons(worker_listen_port);
    worker_address.sin_addr.s_addr = htonl(INADDR_ANY);

    /* Bind the address to the socket */
    if (bind(worker_socket, (struct sockaddr *)&worker_address, sizeof(worker_address)) < 0) {
        printf("Could not bind in worker: %s\n", strerror(errno));
        close(worker_socket);
        exit(1);
    }
    
    /* Set the size of addr_len */
    addr_len = sizeof(struct sockaddr);

    /* Forever, receive messages. Only allow MSG_RESULT and MSG_FINISHED, ignore all others */
    while(1) {

        /* Receive the next message */
        if (recvfrom(worker_socket, &result, sizeof(struct dr_short_msg), 0, (struct sockaddr *)&peer_address, &addr_len) < 0) {
            printf("Could not receive message in results: %s\n", strerror(errno));
            close(worker_socket);
            exit(1);
        }

        /* Check for magic value, and act on the type */
        if (result.dr_magic != DR_MAGIC) {
            close(worker_socket);
            exit(1);
        }

        /* MSG_RESULT: */
        if (result.dr_type == MSG_RESULT) { 
            /* Write map entry to the proper location in the map file */
            lseek(map_fd, result.dr_number * 16, SEEK_SET);
            write(map_fd, result.dr_map_entry, sizeof(char) * 16);
        }
        /* MSG_FINISHED: */
        else if (result.dr_type == MSG_FINISHED) {
            /* Remove the worker who sent this message from the busy workers list, using
                peer_address.sin_port and peer_address.sin_addr.s_addr as the key */
            finished_worker = sock_busy_remove(&busy_workers, peer_address.sin_port, peer_address.sin_addr.s_addr);

            /* Add the worker removed from the busy workers list to the available workers queue */
            pthread_mutex_lock(&wq_mutex);
            sock_enqueue(&workerq, finished_worker);
            pthread_mutex_unlock(&wq_mutex);

            /* Signal wq_cond and busy_cond. workerq is non-empty for sure now,
                busy_workers might not be, but the tasks thread loops, checking this.
                If nobody is waiting, then nothin will happen, so it is safe to signal
                after every MSG_FINISHED message. */
                pthread_cond_signal(&wq_cond);
                pthread_cond_signal(&busy_cond);
        } 
    }

    close(worker_socket);
}



int main(int argc, char *argv[]) {

    int requests_socket;		// Socket used to receive UDP requests
    struct sockaddr_in requests_address;// Our listening address
    struct sockaddr_in requestor;	// Address of the socket we received a request from
    struct sockaddr_in *new_worker;	// Pointer for dynamically allocated new worker
    socklen_t requestor_size;		// Used by recvfrom
    struct dr_short_msg request_msg;    // Buffer to receive the request message
    struct dr_short_msg reply_msg;      // Buffer to hold reply message
    int num_workers = 0;		// Number of workers who have joined

    pthread_t file_transfer_thread;     // File transfer thread
    pthread_t task_thread;              // Map task sending thread
    pthread_t result_thread;            // Result processing thread


    if (argc < 3) {
        printf("Usage: %s requests_port worker_port\n", argv[0]);
        exit(0);
    }

    /* Try to convert argv[1] to requests port number */
    req_listen_port = (uint16_t) (strtol(argv[1], NULL, 0) & 0xffff);
    if (req_listen_port == 0) {
        printf("Invalid requests port: %s\n", argv[1]);
        exit(1);
    }

    /* Try to convert argv[2] to worker port number */
    worker_listen_port = (uint16_t) (strtol(argv[2], NULL, 0) & 0xffff);
    if (worker_listen_port == 0) {
        printf("Invalid worker port: %s\n", argv[2]);
        exit(1);
    }

    /* Initialize semaphores. They will be used to wait for an event */
    sem_init(&sem_classification, 0, 0);
    sem_init(&sem_map, 0, 0);
    sem_init(&sem_worker, 0, 0);

    /* Initialize available worker queue */
    init_sock_queue(&workerq);

    /* Create the threads. We are not interested in a return value */
    pthread_create(&file_transfer_thread, NULL, do_transfers, NULL);
    pthread_create(&task_thread, NULL, do_tasks, NULL);
    pthread_create(&result_thread, NULL, do_results, NULL);

    pthread_detach(file_transfer_thread);
    pthread_detach(task_thread);
    pthread_detach(result_thread);

    /* Create UDP socket to listen for requests */
    requests_socket = socket(AF_INET, SOCK_DGRAM, 0);
    if (requests_socket < 0) {
        printf("Could not create socket in server: %s\n", strerror(errno));
        exit(1);
    }

    /* Zero out requests address socket and set fields to listen on all available interfaces
       on the requests UDP port */
    memset(&requests_address, 0, sizeof(requests_address));
    requests_address.sin_family = AF_INET;
    requests_address.sin_port = htons(req_listen_port);
    requests_address.sin_addr.s_addr = htonl(INADDR_ANY);

    /* Bind the address to the socket */
    if (bind(requests_socket, (struct sockaddr *)&requests_address, sizeof(requests_address)) < 0) {
        printf("Could not bind in server: %s\n", strerror(errno));
        close(requests_socket);
        exit(1);
    }

    /* Zero out reply message and set magic value */
    memset(&reply_msg, 0, sizeof(reply_msg));
    reply_msg.dr_magic = DR_MAGIC;
    
    /* Set the size of requestor_size */
    requestor_size = sizeof(struct sockaddr);

    /* Forever, wait for incoming requests */
    while (1) {

        /* Receive the next request message */
        if (recvfrom(requests_socket, &request_msg, sizeof(struct dr_short_msg), 0, (struct sockaddr *)&requestor, &requestor_size) < 0) {
            printf("Could not receive message in server: %s\n", strerror(errno));
            close(requests_socket);
            exit(1);
        }

        /* Check for magic value and types MSG_REQUEST or MSG_JOIN. Ignore otherwise */
        if (request_msg.dr_magic == DR_MAGIC && (request_msg.dr_type == MSG_REQUEST || request_msg.dr_type == MSG_JOIN)) {

            /* If the message was MSG_REQUEST */
            if (request_msg.dr_type == MSG_REQUEST) {
                /* If we are currently not busy: */
                if (!busy) {
                    /* Set class_file_size to the size of the classification file */
                    class_file_size = request_msg.dr_number;

                    /* Set busy to 1 */
                    busy = 1;

                    /* Prepare and send MSG_ACCEPT message */
                    reply_msg.dr_type = MSG_ACCEPT;
                    if (sendto(requests_socket, &reply_msg, sizeof(struct dr_short_msg), 0, (struct sockaddr *)&requestor, sizeof(requestor)) < 0) {
                        printf("Could not send in server (accept): %s\n", strerror(errno));
                        close(requests_socket);
                        exit(1);
                    }
                }
                /* If we are busy: */
                if (busy) {
                    /* Prepare and send MSG_DENY message */
                    reply_msg.dr_type = MSG_DENY;
                    if (sendto(requests_socket, &reply_msg, sizeof(struct dr_short_msg), 0, (struct sockaddr *)&requestor, sizeof(requestor)) < 0) {
                        printf("Could not send in server (deny): %s\n", strerror(errno));
                        close(requests_socket);
                        exit(1);
                    }

                }

            }

            /* If the message was MSG_JOIN */
            if (request_msg.dr_type == MSG_JOIN) {
                /* Dynamically allocate a new struct sockaddr_in */
                new_worker = malloc(sizeof(struct sockaddr_in));
                if (!new_worker) {
                    printf("Failed to malloc: %s\n", strerror(errno));
                    close(requests_socket);
                    exit(1);
                }

                /* Zero out the new struct and copy the requestor struct sockaddr
                   information here, so that we remember it */
                memset(new_worker, 0, sizeof(struct sockaddr_in));
                memcpy(new_worker, &requestor, sizeof(*new_worker));

                /* Generate and send MSG_ACCEPT message to new worker */
                reply_msg.dr_type = MSG_ACCEPT;
                if (sendto(requests_socket, &reply_msg, sizeof(struct dr_short_msg), 0, (struct sockaddr *)new_worker, sizeof(*new_worker)) < 0) {
                    printf("Could not send in server (join): %s\n", strerror(errno));
                    close(requests_socket);
                    exit(1);
                }

                /* Add the new worker to the available worker queue */
                if (sock_enqueue(&workerq, new_worker) < 0) {
                    printf("Could not enqueue new worker.\n");
                    close(requests_socket);
                    exit(1);
                }
                /* Signal wq_cond, just in case the tasks thread is waiting on an empty queue */
                pthread_cond_signal(&wq_cond);

                /* If this is the very first worker to join, set busy to zero, and signal
                   the tasks thread. */
                if (num_workers == 0) {
                    busy = 0;
                    sem_post(&sem_worker);
                }
                /* Increase worker count */
                num_workers++;
            }
        }
    }

    close(requests_socket);
}
