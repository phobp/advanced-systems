/* network.h */

#ifndef __CS361_NETWORK__
#define __CS361_NETWORK__

#include <stdint.h>

#define MSG_REQUEST 1
#define MSG_JOIN 2
#define MSG_ACCEPT 3
#define MSG_DENY 4
#define MSG_MAP 5
#define MSG_RESULT 6
#define MSG_FINISHED 7

#define DR_MAGIC 0x31363343
#define DR_CLASSIFICATION_CHUNK 60000

struct dr_short_msg {
    uint32_t dr_magic;
    uint32_t dr_type;
    uint32_t dr_number;
    unsigned char dr_map_entry[16];
};

struct dr_long_msg {
    uint32_t dr_magic;
    uint32_t dr_type;
    uint32_t dr_number;
    char dr_filename[12];
    unsigned char dr_data[DR_CLASSIFICATION_CHUNK];
};

#endif
