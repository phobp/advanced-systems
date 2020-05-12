/* project3.c 

    Name: Brendan Pho
    This work adheres to the JMU honor code.
*/

#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>
#include <stdint.h>
#include "common.h"
#include "classify.h"

int main(int argc, char *argv[])
{
    int input_fd;
    int classification_fd;
    int map_fd;
    int bytes_read;
    int i = 0;
    int j = 1;
    int max = 1;
    int temp;
    uint32_t rel_clust_num_jpg = 0;
    uint32_t rel_clust_num_htm = 0;
    unsigned char classification, cluster_type;
    unsigned char prev_class = TYPE_UNCLASSIFIED;
    char cluster_data[CLUSTER_SIZE];
    char file_name[13];
    file_name[12] = '\0';
    
    // We only accept running with one command line argumet: the name of the
    // data file
    if (argc != 2) {
        printf("Usage: %s data_file\n", argv[0]);
        return 1;
    }
    
    // Try opening the file for reading, exit with appropriate error message
    // if open fails
    input_fd = open(argv[1], O_RDONLY);
    if (input_fd < 0) {
        printf("Error opening file \"%s\" for reading: %s\n", argv[1], strerror(errno));
        return 1;
    }
    
    // Iterate through all the clusters, reading their contents
    // into cluster_data
  
    classification_fd = open(CLASSIFICATION_FILE, O_WRONLY | O_CREAT);
    while ((bytes_read = read(input_fd, &cluster_data, CLUSTER_SIZE)) > 0) {
        assert(bytes_read == CLUSTER_SIZE);
        classification = TYPE_UNCLASSIFIED;
        
        /*
            In this loop, you need to implement the functionality of the
            classifier. Each cluster needs to be examined using the functions
            provided in classify.c. Then for each cluster, the attributes
            need to be written to the classification file.
        */

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

        if (write(classification_fd, &classification, sizeof(classification)) < 0) {
            printf("Failed to write to classification file.");
        }
       
    }
    close(classification_fd);
    close(input_fd);

    // Try opening the classification file for reading, exit with appropriate
    // error message if open fails 
    classification_fd = open(CLASSIFICATION_FILE, O_RDONLY); // Instead of opening this file here, you may elect to open it before the classifier loop in read/write mode
    map_fd = open(MAP_FILE, O_WRONLY | O_CREAT);

    if (classification_fd < 0) {
        printf("Error opening file \"%s\": %s\n", CLASSIFICATION_FILE, strerror(errno));
        return 1;
    }
    
    if (map_fd < 0) {
        printf("Error opening file \"%s\": %s\n", MAP_FILE, strerror(errno));
        return 1;
    }
    // Iterate over each cluster, reading in the cluster type attributes
    while ((bytes_read = read(classification_fd, &cluster_type, 1)) > 0) {
        /*
            In this loop, you need to implement the functionality of the
            mapper. For each cluster, a map entry needs to be created and
            written to the map file.
        */
        if (cluster_type & (TYPE_IS_JPG | TYPE_JPG_HEADER | TYPE_JPG_FOOTER)) {
            j = i;
            i = max;
            max++;
            rel_clust_num_jpg = 0;
        } 


        // Increment file number when reaching a jpg header
        else if ((cluster_type & TYPE_JPG_HEADER) || (cluster_type & TYPE_HTML_HEADER)) {    
            j = i;
            i = max;
            max++;
        } 

        // When a jpg (or html) changes to an html (jpg)
        else if ((prev_class & cluster_type) == 0 && prev_class != TYPE_UNCLASSIFIED) {
            temp = i;
            i = j;
            j = temp;
            max++;
        }

        // Print the name of the jpg file and the count of the file
        if (cluster_type & TYPE_IS_JPG) {
            if ((cluster_type & TYPE_JPG_HEADER) == 0) {
                rel_clust_num_jpg++;
            }
            sprintf(file_name, "file%04d.jpg", i); 
        }

        // Print the name of the html file and the count of the file
        if (cluster_type & TYPE_IS_HTML) {
            if ((cluster_type & TYPE_HTML_HEADER) == 0) {
                rel_clust_num_htm++;
            }
            sprintf(file_name, "file%04d.htm", i); 
        }

        // Write to the map file
        write(map_fd, file_name, sizeof(char) * 12);   
        write(map_fd, (cluster_type & TYPE_IS_HTML) ? &rel_clust_num_htm : &rel_clust_num_jpg, sizeof(uint32_t));
       
        // Increase the file count and reset the relative cluster number to 0
        if (cluster_type & TYPE_JPG_FOOTER ) { 
            i = max;
            max++;
            rel_clust_num_jpg = 0;
        }
        
        // Increase the file count and reset the relative cluster number to 0
        if (cluster_type & TYPE_HTML_FOOTER) { 
            i = max;
            max++;
            rel_clust_num_htm = 0;
        }

        prev_class = cluster_type;
    }
    
    close(classification_fd);
    close(map_fd);

    return 0;
}
