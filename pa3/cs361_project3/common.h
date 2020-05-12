/* common.h */

#ifndef __CS361_COMMON__
#define __CS361_COMMON__

#define CLUSTER_SIZE 4096

#define TYPE_UNCLASSIFIED 0
#define TYPE_IS_JPG 1
#define TYPE_JPG_HEADER 2
#define TYPE_JPG_FOOTER 4
#define TYPE_IS_HTML 8
#define TYPE_HTML_HEADER 16
#define TYPE_HTML_FOOTER 32
#define TYPE_UNKNOWN 128

#define CLASSIFICATION_FILE "classification"
#define MAP_FILE "map"

#endif
