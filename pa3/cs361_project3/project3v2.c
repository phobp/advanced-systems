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
    int jpg_count = 0;
    int html_count = 0;

    uint32_t rel_clust_num_jpg = 0;
    uint32_t rel_clust_num_htm = 0;
    unsigned char classification, cluster_type;
    char cluster_data[CLUSTER_SIZE];
    char file_name[13];
    file_name[12] = '\0'; 
    
    // We only accept running with one command line argument: the name of the
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

    classification_fd = open(CLASSIFICATION_FILE, O_WRONLY | O_CREAT, 0666);
  
    while ((bytes_read = read(input_fd, &cluster_data, CLUSTER_SIZE)) > 0) {
        assert(bytes_read == CLUSTER_SIZE);
        classification = TYPE_UNCLASSIFIED;
        
        /*
            In this loop, you need to implement the functionality of the
            classifier. Each cluster needs to be examined using the functions
            provided in classify.c. Then for each cluster, the attributes
            need to be written to the classification file.
        */

        // Checks for jpg header
        if (has_jpg_header(cluster_data)) {
            classification |= TYPE_IS_JPG;
            classification |= TYPE_JPG_HEADER;
        }
        
        // Checks for jpg footer
        if (has_jpg_footer(cluster_data)) {
            classification |= TYPE_IS_JPG;
            classification |= TYPE_JPG_FOOTER;
        }

        // Checks for jpg body
        if (has_jpg_body(cluster_data)) {classification |= TYPE_IS_JPG;}
        
        // Checks for html header
        if (has_html_header(cluster_data)) {
            classification |= TYPE_IS_HTML;
            classification |= TYPE_HTML_HEADER;
        }

        // Checks for html footer
        if (has_html_footer(cluster_data)) {
            classification |= TYPE_IS_HTML;
            classification |= TYPE_HTML_FOOTER;
        }

        // Checks for html body
        if (has_html_body(cluster_data)) {classification |= TYPE_IS_HTML;}
        
        // Writes to classification file
        if (write(classification_fd, &classification, sizeof(classification)) < 0) {
            printf("Failed to write to classification file.");
        }

    }

    close(classification_fd);
    close(input_fd);

    // Try opening the classification file for reading, exit with appropriate
    // error message if open fails 
    classification_fd = open(CLASSIFICATION_FILE, O_RDONLY); // Instead of opening this file here, you may elect to open it before the classifier loop in read/write mode
    map_fd = open(MAP_FILE, O_WRONLY | O_CREAT, 0666);

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

        // Checks if the type is a jpg 
        if (cluster_type < TYPE_IS_HTML) { 
            
            // Checks if the type is a jpg header
            if (cluster_type & TYPE_JPG_HEADER && cluster_type & TYPE_IS_JPG) {    
                if (jpg_count < html_count) {
                    jpg_count = html_count;
                }

                jpg_count++;
                rel_clust_num_jpg = 0;
                sprintf(file_name, "file%04d.jpg", jpg_count);       
            }
            
            // Checks if the type is a jpg body
            else if (cluster_type & TYPE_IS_JPG) {
              
                rel_clust_num_jpg++;
                sprintf(file_name, "file%04d.jpg", jpg_count);
            }

            // Checks if the type is a jpg footer
            else if (cluster_type & TYPE_JPG_FOOTER && cluster_type & TYPE_IS_JPG) {
                if (jpg_count < html_count) {
                    jpg_count = html_count;
                }
                rel_clust_num_jpg++;
                sprintf(file_name, "file%04d.jpg", jpg_count);
            }

            // Checks if the type is a full jpg
            else if (cluster_type & TYPE_JPG_HEADER && cluster_type & TYPE_IS_JPG && cluster_type & TYPE_JPG_FOOTER) {
                if (jpg_count < html_count) {
                    jpg_count = html_count;
                }
                jpg_count++;
                rel_clust_num_jpg = 0;
                sprintf(file_name, "file%04d.jpg", jpg_count);
            }
            
            // Writes the file name and cluster number bytes
            write(map_fd, file_name, sizeof(char) * 12);   
            write(map_fd, &rel_clust_num_jpg, sizeof(uint32_t)); 

        // All html conditions are here
        } else {
            
            // Checks if the type is an html header
            if (cluster_type & TYPE_HTML_HEADER && cluster_type & TYPE_IS_HTML) {    
                if (html_count < jpg_count) {
                    html_count = jpg_count;
                }
                 
                html_count++;
                rel_clust_num_htm = 0;
                sprintf(file_name, "file%04d.htm", html_count);
            }

            // Checks if the type is an html body
            else if (cluster_type & TYPE_IS_HTML) {
               
                rel_clust_num_htm++; 
                sprintf(file_name, "file%04d.htm", html_count);
            }

            // Checks if the type is an html footer
            else if (cluster_type & TYPE_HTML_FOOTER && cluster_type & TYPE_IS_HTML) {
                if (html_count < jpg_count) {
                    html_count = jpg_count;
                }
                rel_clust_num_htm++;
                sprintf(file_name, "file%04d.htm", html_count);
            }

            // Checks if the type is a full html
            else if (cluster_type & TYPE_HTML_HEADER && cluster_type & TYPE_IS_HTML && cluster_type & TYPE_HTML_FOOTER) {
                if (html_count < jpg_count) {
                    html_count = jpg_count;
                }
                html_count++;
                rel_clust_num_htm = 0;
                sprintf(file_name, "file%04d.htm", html_count);      
            }
            
            // Writes the file name and cluster number bytes
            write(map_fd, file_name, sizeof(char) * 12);   
            write(map_fd, &rel_clust_num_htm, sizeof(uint32_t)); 
        }
    }
    
    close(classification_fd);
    close(map_fd);

    return 0;
}
