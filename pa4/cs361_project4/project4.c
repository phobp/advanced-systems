/* project4.c

Name: Brendan Pho
Honor code statement: This work adheres to the JMU honor code.

*/

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>
#include <stdlib.h>

#include "common.h"
#include "classify.h"
#ifdef GRADING // do not delete this or the next two lines
#include "gradelib.h"
#endif


#define NUM_PROCESSES 5

char classify(char *);
// This is the recommended struct for sending a message about
// a cluster's type through the pipe. You need not use this,
// but I would recommend that you do.
struct msg {
    int msg_cluster_number;  // The cluster number from the input file
    unsigned char msg_cluster_type;  // The type of the above cluster
                                     // as determined by the classifier
};

int main(int argc, char *argv[])
{
    int input_fd;
    int classification_fd;
    pid_t pid;
    int pipefd[2];
    struct msg message;
    int start_cluster; // When a child process is created, this variable must
                       // contain the first cluster in the input file it
                       // needs to classify
    int clusters_to_process; // This variable must contain the number of
                             // clusters a child process needs to classify

    // The user must supply a data file to be used as input
    if (argc != 2) {
        printf("Usage: %s data_file\n", argv[0]);
        return 1;
    }

    // Open classification file for writing. Create file if it does not
    // exist. Exit with error if open() fails.
    classification_fd = open(CLASSIFICATION_FILE, O_WRONLY | O_CREAT, 0600);
    if (classification_fd < 0) {
        printf("Error creating file \"%s\": %s\n", CLASSIFICATION_FILE, strerror(errno));
        return 1;
    }

    // Create the pipe here. Exit with error if this fails.
    
    if(pipe(pipefd) == -1) {
        printf("Failed to create pipe: %s\n", strerror(errno));
        return 1;
    }
    
    // The pipe must be created at this point
#ifdef GRADING // do not delete this or you will lose points
    test_pipefd(pipefd, 0);
#endif

    start_cluster = 0; // Always start at cluster 0
    clusters_to_process = 0; 

    input_fd = open(argv[1], O_RDONLY);

    // Buffer that will be passed into the classify() function
    char cluster_data[CLUSTER_SIZE];

    // Get the number of clusters in a file
    int num_clusters = lseek(input_fd, 0, SEEK_END) / CLUSTER_SIZE;

    // r will be the remainder of the number of clusters distributed
    // among the processes
    int r = num_clusters % NUM_PROCESSES;

    // q will be the number of clusters distributed evently
    // among the processes
    int q = num_clusters / NUM_PROCESSES;

    close(input_fd);

    // Fork NUM_PROCESS number of child processes
    for (int i = 0; i < NUM_PROCESSES; i++) {
        
        start_cluster += clusters_to_process;
        clusters_to_process = q;

        if (i < r) {
            clusters_to_process++;
        }

        pid = fork();
        // Exit if fork() fails.
        if (pid == -1)
            exit(1);
        // This is the place to write the code for the child processes
        else if (pid == 0) {

            // In this block, you need to implement the entire logic
            // for the child processes to be aware of which clusters
            // they need to process, classify them, and create a message
            // for each cluster to be written to the pipe.
            close(pipefd[0]);
            
            // Open the input file and seek to the starting cluster positions
            input_fd = open(argv[1], O_RDONLY);
            lseek(input_fd, start_cluster * CLUSTER_SIZE, SEEK_SET);


#ifdef GRADING // do not delete this or you will lose points
            printf("Child process %d\n\tStart cluster: %d\n\tClusters to process: %d\n",
                    getpid(), start_cluster, clusters_to_process);
#endif


            // At this point the pipe must be fully set up for the child
            // This code must be executed before you start iterating over the input
            // file and before you generate and write messages.
#ifdef GRADING // do not delete this or you will lose points
           test_pipefd(pipefd, getpid());
#endif


            // Implement the main loop for the child process below this line

            // Set the attributes of the msg struct and write the message
            // to the pipe
            for (int j = start_cluster; j < clusters_to_process + start_cluster; j++) {
                read(input_fd, &cluster_data, CLUSTER_SIZE);
                message.msg_cluster_number = j;
                message.msg_cluster_type = classify(cluster_data);

                if (write(pipefd[1], &message, sizeof(struct msg)) < 0) {
                    printf("Failed to write to classification file.");
                }
            }     

            exit(0); // This line needs to be the last one for the child
            // process code. Do not delete this!
        }

    }

    // All the code for the parent's handling of the messages and
    // creating the classification file needs to go in the block below

    close(pipefd[1]);
    // At this point, the pipe must be fully set up for the parent
#ifdef GRADING // do not delete this or you will lose points
    test_pipefd(pipefd, 0);
#endif

    // Read one message from the pipe at a time
    while (read(pipefd[0], &message, sizeof(message)) > 0) {
        // In this loop, you need to implement the processing of
        // each message sent by a child process. Based on the content,
        // a proper entry in the classification file needs to be written.

        // Seek to the cluster positions and write cluster data to the 
        // classification file
        lseek(classification_fd, message.msg_cluster_number, SEEK_SET);
        write(classification_fd, &message.msg_cluster_type, sizeof(unsigned char));  
    }

    close(classification_fd); // close the file descriptor for the
                              // classification file
    return 0;
};

// Implements the functionality of the classifier
char classify(char *cluster_data) {
    char classification = TYPE_UNCLASSIFIED;

    if (has_jpg_header(cluster_data)) {
        classification |= TYPE_IS_JPG;
        classification |= TYPE_JPG_HEADER;
    }

    if (has_jpg_footer(cluster_data)) {
        classification |= TYPE_IS_JPG;
        classification |= TYPE_JPG_FOOTER;
    }

    if (has_jpg_body(cluster_data)) {classification |= TYPE_IS_JPG;}

    if (has_html_header(cluster_data)) {
        classification |= TYPE_IS_HTML;
        classification |= TYPE_HTML_HEADER;
    }

    if (has_html_footer(cluster_data)) {
        classification |= TYPE_IS_HTML;
        classification |= TYPE_HTML_FOOTER;
    }

    if (has_html_body(cluster_data)) {classification |= TYPE_IS_HTML;}

    return classification;
}
