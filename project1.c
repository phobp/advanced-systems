/*
	Project 1
	Name: Brendan Pho
	This work adheres to the JMU honor code
*/

#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

// Represents a file entry to add to the linked list
struct file_entry {
    int fd; // Stores a file descriptor
    char fn[13]; // Stores a file name
    struct file_entry *next; // The next file entry
    struct file_entry *prev; // The previous file entry
};

// Represents the linked list
struct file_list {
    struct file_entry *head;
    struct file_entry *tail; 
};

// Makes space for a file_list
struct file_list *create_list() {

    return calloc(1, sizeof(struct file_list));
}

// Searches the list using the file name and returns a file descriptor
int search(struct file_list *list, char *fn) {
    struct file_entry *curr = NULL;
    for (curr = list->head; curr != list->tail; curr = curr->next ) {
        if (strncmp(curr->fn, fn, 12) == 0) {
            return curr->fd;
        }
    }

    return -1;
}

// Determines if the list is empty
bool is_empty(struct file_list *list) {
    return list->head == NULL && list->tail == NULL;
}

// Adds a file name and file descriptor to the list
void add_fn(char *fn, int fd, struct file_list *list) {
    
    struct file_entry *new;

    new = calloc(1, sizeof(struct file_entry));
    
    memcpy(new->fn, fn, 12);
    new->fd = fd;

    if (is_empty(list)) {
        list->head = new;
        list->tail = list->head;
    } else {
        list->tail->next = new;
        list->tail = new;
    }
}

// Frees memory
void clean(struct file_list *list) {
    
    struct file_entry *curr = list->head;
    struct file_entry *prev = NULL;
    while (curr != NULL) {
        if (prev != NULL) {
            close(prev->fd);
            free(prev);
        }
        
        prev = curr;
        curr = curr->next;
    }

    free(list);

}

int main(int argc, char *argv[]) {

    char fn[13]; // Stores the file names
    uint32_t cluster_num; // Stores the cluster numbers
    int i = 0; // Index of cluster
    const uint32_t CLUSTER_SIZE = 4096; // Constant size of clusters
    char buf[4096]; // Buffer for cluster

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

    struct file_list *list = create_list();

    // Iterates through the map file
    while (1)
    { 
        memset(&fn, 0, 13); 
         
        int filename_len = read(fdmap, &fn, sizeof(char) * 12); // Gets the file name
        int cluster_offset = read(fdmap, &cluster_num, sizeof(cluster_num)); // Gets the cluster number             
        if (filename_len <= 0 || cluster_offset <= 0) {
            break;
        }
        
        int fdx = search(list, fn);
        if (fdx == -1) {
            fdx = open(fn, O_RDWR | O_CREAT, 0700); // Get file descriptor of file X
            add_fn(fn, fdx, list);
        }
         
        lseek(fdx, cluster_num * CLUSTER_SIZE, SEEK_SET); 
        lseek(fdinput, i * CLUSTER_SIZE, SEEK_SET);
        read(fdinput, buf, CLUSTER_SIZE); // Read cluster into buffer
        write(fdx, buf, 4096); // Write to files
        
        i += 1;
    }

    clean(list);
}

