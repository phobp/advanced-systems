/* project5.c

Name: Brendan Pho
Honor code statement: This work adheres to the JMU honor code.

*/

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <mqueue.h>

#include "common.h"
#include "classify.h"
#include "intqueue.h"
#ifdef GRADING // do not delete this or the next two lines
#include "gradelib.h"
#endif

int main(int argc, char *argv[])
{
    int input_fd;
    int classification_fd;
    int map_fd;
    pid_t pid;
    off_t file_size;
    mqd_t tasks_mqd, results_mqd; // message queue descriptors
    char tasks_mq_name[16]; // name of the tasks queue
    char results_mq_name[18];   // name of the results queue
    int num_clusters;
    // New variables
    ssize_t n;
    int child_pid[NUM_PROCESSES]; // stores the different child pid's

    struct mq_attr blocking; // sets attribute struct to blocking mode
    blocking.mq_flags = 0;
    blocking.mq_maxmsg = 1000; 
    blocking.mq_msgsize = MESSAGE_SIZE_MAX;

    struct mq_attr nonblocking; // sets attribute struct to nonblocking mode
    nonblocking.mq_flags = O_NONBLOCK;
    nonblocking.mq_maxmsg = 1000;
    nonblocking.mq_msgsize = MESSAGE_SIZE_MAX;

    if (argc != 2) {
        printf("Usage: %s data_file\n", argv[0]);
        return 1;
    }

    input_fd = open(argv[1], O_RDONLY);
    if (input_fd < 0) {
        printf("Error opening file \"%s\" for reading: %s\n", argv[1], strerror(errno));
        return 1;
    }

    // Determine the file size of the input file
    file_size = lseek(input_fd, 0, SEEK_END);
    close(input_fd);
    // Calculate how many clusters are present
    num_clusters = file_size / CLUSTER_SIZE;

    // Generate the names for the tasks and results queue
    snprintf(tasks_mq_name, 16, "/tasks_%s", getlogin());
    tasks_mq_name[15] = '\0';
    snprintf(results_mq_name, 18, "/results_%s", getlogin());
    results_mq_name[17] = '\0';

    // Create the child processes
    for (int i = 0; i < NUM_PROCESSES; i++) {
        pid = fork();
        if (pid == -1)
            exit(1);
        else if (pid == 0) {

            // All the worker code must go here

            char cluster_data[CLUSTER_SIZE];
            char recv_buffer[MESSAGE_SIZE_MAX];
            struct task *my_task;
            struct result my_result;
            unsigned char classification;
            char char_buff;
            char footer;
            char body;

            // Here you need to create/open the two message queues for the
            // worker process. You must use the tasks_mqd and results_mqd
            // variables for this purpose
            
            // open the tasks message queue decriptor
            if ((tasks_mqd = mq_open(tasks_mq_name, O_RDONLY | O_CREAT, 0666, &blocking)) < 0) {
                printf("Error opening tasks message queue: %s\n", strerror(errno));
                return 1;
            }
            
            // open the results message queue descriptor
            if ((results_mqd = mq_open(results_mq_name, O_WRONLY | O_CREAT, 0666, &blocking)) < 0) {
                printf("Error opening results message queue: %s\n", strerror(errno));
                return 1;
            }
            
            // open the input file
            input_fd = open(argv[1], O_RDONLY);

            // open the classification file
            classification_fd = open(CLASSIFICATION_FILE, O_RDWR | O_CREAT, 0600);

            // open the map file
            map_fd = open(MAP_FILE, O_WRONLY | O_CREAT, 0600);
            if (!map_fd) {
                printf("Error opening map file: %s", strerror(errno));
                exit(1);
            }
            // At this point, the queues must be open
#ifdef GRADING // do not delete this or you will lose points
            test_mqdes(tasks_mqd, "Tasks", getpid());
            test_mqdes(results_mqd, "Results", getpid());
#endif

            // A worker process must endlessly receive new tasks
            // until instructed to terminate
            while(1) {

                // receive the next task message here
                n = mq_receive(tasks_mqd, recv_buffer, MESSAGE_SIZE_MAX, NULL);
                if (n < 0) {
                    printf ("Error receiving message: %s\n", strerror(errno));
                    return 1;
                }

                // cast the received message to a struct task
                my_task = (struct task *)recv_buffer;
                switch (my_task->task_type) {

                    case TASK_CLASSIFY:

#ifdef GRADING // do not delete this or you will lose points
                        printf("(%qd): received TASK_CLASSIFY\n", getpid());
#endif
                        // you must retrieve the data for the specified cluster
                        // and store it in cluster_data before executing the
                        // code below
                        lseek(input_fd, my_task->task_cluster * CLUSTER_SIZE, SEEK_SET);
                        read(input_fd, cluster_data, CLUSTER_SIZE);

                        // Classification code
                        classification = TYPE_UNCLASSIFIED;
                        if (has_jpg_body(cluster_data))
                            classification |= TYPE_IS_JPG;
                        if (has_jpg_header(cluster_data))
                            classification |= TYPE_JPG_HEADER | TYPE_IS_JPG;
                        if (has_jpg_footer(cluster_data))
                            classification |= TYPE_JPG_FOOTER | TYPE_IS_JPG;
                        if (classification == TYPE_UNCLASSIFIED) {
                            if (has_html_body(cluster_data))
                                classification |= TYPE_IS_HTML;
                            if (has_html_header(cluster_data))
                                classification |= TYPE_HTML_HEADER | TYPE_IS_HTML;
                            if (has_html_footer(cluster_data))
                                classification |= TYPE_HTML_FOOTER | TYPE_IS_HTML;
                        }
                        if (classification == TYPE_UNCLASSIFIED)
                            classification = TYPE_UNKNOWN;

                        // prepare a results message and send it to results
                        // queue here
                        my_result.res_cluster_number = my_task->task_cluster;
                        my_result.res_cluster_type = classification;
                        
                        // generates classification message and sends to result queue
                        if (mq_send(results_mqd, (char *)&my_result, MESSAGE_SIZE_MAX, 0) < 0) {
                            printf("Failed to send message: %s", strerror(errno));
                            return 1;
                        }

                        break;

                    case TASK_MAP:
                    {
#ifdef GRADING // do not delete this or you will lose points
                        printf("(%d): received TASK_MAP\n", getpid());
#endif

                        // implement the map task logic here 
                        int cluster_num = my_task->task_cluster;
                        int relative_cluster_num = 0;

                        lseek(classification_fd, cluster_num, SEEK_SET); 
                        read(classification_fd, &classification, sizeof(unsigned char));

                        // based on header type, set footer and body variables for comparison later
                        if (classification & TYPE_JPG_HEADER) {
                            footer = TYPE_JPG_FOOTER;
                            body = TYPE_IS_JPG | TYPE_JPG_FOOTER | TYPE_JPG_HEADER;
                        } else {
                            footer = TYPE_HTML_FOOTER;
                            body = TYPE_IS_HTML | TYPE_HTML_FOOTER | TYPE_HTML_HEADER;
                        }

                        char_buff = classification;
                        
                        while (1) {

                            // write to the specific cluster number when header or body
                            if ((char_buff & body) > 0) {
                                lseek(map_fd, cluster_num * 16, SEEK_SET);
                                write(map_fd, my_task->task_filename, sizeof(char) * 12);
                                write(map_fd, &relative_cluster_num, sizeof(int));
                                relative_cluster_num++;
                            }
                            
                            // break if a footer is encountered
                            if ((char_buff & footer) > 0) {
                                break;
                            }

                            read(classification_fd, &char_buff, sizeof(unsigned char));
                            cluster_num++;
                        } 
                        break;
                    }
                    default:

#ifdef GRADING // do not delete this or you will lose points
                        printf("(%d): received TASK_TERMINATE or invalid task\n", getpid());
#endif

                        // implement the terminate task logic here
                        close(classification_fd);
                        close(input_fd);
                        close(map_fd);
                        mq_close(tasks_mqd);
                        mq_close(results_mqd);
                        exit(0);
                }
            }
        } else {

            // assigns child pid's to the array to wait on them later
            child_pid[i] = pid;
        }
    }

    // All the supervisor code needs to go here
    
    struct intqueue headerq;

    // Initialize an empty queue to store the clusters that have file headers.
    // This queue needs to be populated in Phase 1 and worked off in Phase 2.
    initqueue(&headerq);

    
    // Here you need to create/open the two message queues for the
    // supervisor process. You must use the tasks_mqd and results_mqd
    // variables for this purpose

    // open tasks message queue descriptor
    if ((tasks_mqd = mq_open(tasks_mq_name, O_WRONLY | O_CREAT, 0666, &nonblocking)) < 0) {
        printf("Error opening tasks message queue: %s\n", strerror(errno));
        return 1;
    }

    // open results message queue descriptor
    if ((results_mqd = mq_open(results_mq_name, O_RDONLY | O_CREAT, 0666, &blocking)) < 0) {
        printf("Error opening results message queue: %s\n", strerror(errno));
        return 1;
    }


    // At this point, the queues must be open
#ifdef GRADING // do not delete this or you will lose points
    test_mqdes(tasks_mqd, "Tasks", getpid());
    test_mqdes(results_mqd, "Results", getpid());
#endif

    // Implement Phase 1 here
    
    // loop around every cluster and generate a task to classify each clusteri
    mq_setattr(tasks_mqd, &nonblocking, NULL);   

    // open classification file
    classification_fd = open(CLASSIFICATION_FILE, O_RDWR | O_CREAT, 0600);
    if (classification_fd < 0) {
        printf("Error creating file \"%s\": %s\n", CLASSIFICATION_FILE, strerror(errno));
        return 1;
    }

    struct mq_attr results_attr;
    struct result *srm;
    struct task classify_task; 
    char message_buff[MESSAGE_SIZE_MAX];
    int msg_send;
    int j;
    int phase1_counter = 0;
    
    // creates a classify task
    for (int i = 0; i < num_clusters; i++) {
        classify_task.task_type = TASK_CLASSIFY;
        classify_task.task_cluster = i;
        msg_send = mq_send(tasks_mqd, (char *)&classify_task, MESSAGE_SIZE_MAX, 0);
        
        // queue is full
        if (msg_send == -1 && errno == EAGAIN) {                       
            mq_getattr(results_mqd, &results_attr);
            i--;
            j = results_attr.mq_curmsgs; 
            while (j > 0) {
                mq_receive(results_mqd, message_buff, MESSAGE_SIZE_MAX, NULL);
                phase1_counter++;
                srm = (struct result *)message_buff;
                lseek(classification_fd, srm->res_cluster_number, SEEK_SET);
                write(classification_fd, &srm->res_cluster_type, sizeof(unsigned char));
                if (srm->res_cluster_type & TYPE_JPG_HEADER || srm->res_cluster_type & TYPE_HTML_HEADER) {
                    enqueue(&headerq, srm->res_cluster_number);
                }

                j--;
            } 
        }
        

    }

    while (phase1_counter < num_clusters) { 
        mq_receive(results_mqd, message_buff, MESSAGE_SIZE_MAX, NULL);   
        phase1_counter++;
        srm = (struct result *)message_buff;
        lseek(classification_fd, srm->res_cluster_number, SEEK_SET);
        write(classification_fd, &srm->res_cluster_type, sizeof(unsigned char));
        if (srm->res_cluster_type & TYPE_JPG_HEADER || srm->res_cluster_type & TYPE_HTML_HEADER) {
            enqueue(&headerq, srm->res_cluster_number);
        }
    }
    
    
    // End of Phase 1

#ifdef GRADING // do not delete this or you will lose points
    printf("(%d): Starting Phase 2\n", getpid());
#endif

    // Here you need to switch the tasks queue to blocking
    mq_setattr(tasks_mqd, &blocking, NULL);

#ifdef GRADING // do not delete this or you will lose points
    test_mqdes(tasks_mqd, "Tasks", getpid());
#endif


    // Implement Phase 2 here
    struct task map_task;
    int file_num = 1;
    unsigned char classification;    

    // creates the map task
    while (!isempty(&headerq)) {
        map_task.task_type = TASK_MAP;
        map_task.task_cluster = dequeue(&headerq);
        pread(classification_fd, &classification, sizeof(unsigned char), map_task.task_cluster);
        
        // creates the file names based on the header found in classification
        if (classification & TYPE_JPG_HEADER) {
            sprintf(map_task.task_filename, "file%04d.jpg", file_num);
        } else {
            sprintf(map_task.task_filename, "file%04d.htm", file_num);
        }

        file_num++;
        mq_send(tasks_mqd, (char *)&map_task, MESSAGE_SIZE_MAX, 0);
    }
    
    
    // End of Phase 2

#ifdef GRADING // do not delete this or you will lose points
    printf("(%d): Starting Phase 3\n", getpid());
#endif

    // Implement Phase 3 here
    struct task terminate_task;
    
    // send terminate tasks to all children
    terminate_task.task_type = TASK_TERMINATE;
    for (int i = 0; i < NUM_PROCESSES; i++) { 
        mq_send(tasks_mqd, (char *)&terminate_task, MESSAGE_SIZE_MAX, 0);
    }
    
    // wait for all child processes to terminate
    for (int i = 0; i < NUM_PROCESSES; i++) {
        waitpid(child_pid[i], NULL, 0);
    }

    mq_close(tasks_mqd);
    mq_close(results_mqd);
    mq_unlink(tasks_mq_name);
    mq_unlink(results_mq_name);
    return 0;
};
