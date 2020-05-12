/* project6.c

  Name: Brendan Pho
  Honor Code Statement: This work adheres to the JMU honor code.

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
#include <pthread.h>
#include <time.h>
#include "common.h"
#include "classify.h"
#include "intqueue.h"

struct intqueue headerq;
int classifications_remaining; // counter to determine how many classifications there are left to process
static pthread_mutex_t classifications_mutex = PTHREAD_MUTEX_INITIALIZER; // mutex updating and checking the classifications remaining to process
char tasks_mq_name[16];
char results_mq_name[18];

void *process_result(void *arg) {
    
    mqd_t results_mqd;
    struct result *srm;
    char message_buff[MESSAGE_SIZE_MAX];
    int classification_fd;
    // sets the attributes for blocking mode
    struct mq_attr blocking;
    blocking.mq_flags = 0;
    blocking.mq_maxmsg = 1000;
    blocking.mq_msgsize = MESSAGE_SIZE_MAX;

    // open classification file
    classification_fd = open(CLASSIFICATION_FILE, O_WRONLY | O_CREAT, 0600);
    if (classification_fd < 0) {
        printf("Error creating file \"%s\": %s\n", CLASSIFICATION_FILE, strerror(errno));
        return NULL;
    } 
            
    // open the results message queue descriptor
    results_mqd = mq_open(results_mq_name, O_RDONLY | O_CREAT, 0666, &blocking);
    if (results_mqd < 0) {
        printf("Error opening results message queue: %s\n", strerror(errno));
        return NULL;
    }

    while (1) {
        // protect the classifications_remaining shared resource
        pthread_mutex_lock(&classifications_mutex);
        // if the classifications remaining is less than or equal to zero, release key, close descriptors, and return
        if (classifications_remaining <= 0) {
            pthread_mutex_unlock(&classifications_mutex);
            mq_close(results_mqd);
            close(classification_fd);
            return NULL;
        }
        classifications_remaining--;
        pthread_mutex_unlock(&classifications_mutex);
        mq_receive(results_mqd, message_buff, MESSAGE_SIZE_MAX, NULL);
        srm = (struct result *)message_buff;
        lseek(classification_fd, srm->res_cluster_number, SEEK_SET);
        write(classification_fd, &srm->res_cluster_type, sizeof(unsigned char));
        if (srm->res_cluster_type & TYPE_JPG_HEADER || srm->res_cluster_type & TYPE_HTML_HEADER) {
            enqueue(&headerq, srm->res_cluster_number);
        }
    } 
}

int main(int argc, char *argv[])
{
    int input_fd;
    pid_t pid;
    off_t file_size;
    int num_clusters;
    pthread_t processor[NUM_THREADS];

    // New Variables
    int child_pid[NUM_PROCESSES];
    mqd_t tasks_mqd;

    struct mq_attr blocking;
    blocking.mq_flags = 0;
    blocking.mq_maxmsg = 1000;
    blocking.mq_msgsize = MESSAGE_SIZE_MAX;

    if (argc != 2) {
        printf("Usage: %s data_file\n", argv[0]);
        return 1;
    }

    // open input file
    input_fd = open(argv[1], O_RDONLY);
    if (input_fd < 0) {
        printf("Error opening file \"%s\" for reading: %s\n", argv[1], strerror(errno));
        return 1;
    }

    file_size = lseek(input_fd, 0, SEEK_END);
    close(input_fd);

    num_clusters = file_size / CLUSTER_SIZE;
    // the classifications remaining is the same as the number of clusters to process
    classifications_remaining = num_clusters;
    snprintf(tasks_mq_name, 16, "/tasks_%s", getlogin());
    tasks_mq_name[15] = '\0';
    snprintf(results_mq_name, 18, "/results_%s", getlogin());
    results_mq_name[17] = '\0';
    
    if ((tasks_mqd = mq_open(tasks_mq_name, O_WRONLY | O_CREAT, 0666, &blocking)) < 0) {
        printf("Error opening tasks message queue: %s\n", strerror(errno));
        return 1;
    }

    for (int i = 0; i < NUM_PROCESSES; i++) {
        pid = fork();
        if (pid == -1)
            exit(1);
        else if (pid == 0) {
            
            execlp("./worker", "./worker", argv[1], NULL);
            printf("execlp failed: %s\n", strerror(errno));
        }
    }

    // create NUM_THREADS threads
    for (int i = 0; i < NUM_THREADS; i++)
        pthread_create(&(processor[i]), NULL, process_result, NULL);
    
    // Phase 1
    mq_setattr(tasks_mqd, &blocking, NULL);
    
    // open classification file
    int classification_fd = open(CLASSIFICATION_FILE, O_RDONLY | O_CREAT, 0600);
    if (classification_fd < 0) {
        printf("Error creating file \"%s\": %s\n", CLASSIFICATION_FILE, strerror(errno));
        return 1;
    }

    struct task classify_task;
    struct mq_attr attr;
     
    // creates num_clusters classification tasks
    for (int i = 0; i < num_clusters; i++) {
        classify_task.task_type = TASK_CLASSIFY;
        classify_task.task_cluster = i;
        mq_send(tasks_mqd, (char *)&classify_task, MESSAGE_SIZE_MAX, 0);
        mq_getattr(tasks_mqd, &attr);
    }

    // terminate NUM_THREADS threads
    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_join(processor[i], NULL);
    }

    // End of Phase 1
    
    // Phase 2
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

    // Phase 3
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
    mq_unlink(tasks_mq_name);
    mq_unlink(results_mq_name);

    // End of Phase 3

    return 0;
};
