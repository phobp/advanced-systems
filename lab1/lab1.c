/*
    Name: Brendan Pho
    This work adheres to the JMU honor code.
*/

#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

int main (int argc, char **argv)
{
    uint32_t item_one;
    uint8_t item_two;
    uint16_t item_three;
    uint32_t item_four;
    char item_five[2048];
    
    typedef struct
    {
        uint32_t a;
        uint16_t b;
        int16_t c;
        char d[8];
        char *e;
    } item_six;

    uint64_t item_seven;
    uint32_t item_eight;
    char item_nine[5] = {'C', 'S', '3', '6', '1'};

    
    if (argc < 2) {
        printf("File not specified\n");
        exit(0);
    }

    int fd = open(argv[1], O_RDWR);
    if (fd == -1) {
        printf("Error opening file\n");
        exit(0);
    }

    lseek(fd, 32, SEEK_SET);
    read(fd, &item_one, sizeof(uint32_t));
    printf("Item 1:\n");
    printf("\tDecimal: %u\n \tHex: 0x%08x\n", item_one, item_one);

    lseek(fd, 300, SEEK_SET);
    read(fd, &item_two, sizeof(uint8_t));
    printf("Item 2:\n");
    printf("\tDecimal: %u\n \tHex: 0x%08x\n", item_two, item_two);

    lseek(fd, 123, SEEK_CUR);
    read(fd, &item_three, sizeof(uint16_t));
    printf("Item 3:\n");    
    printf("\tDecimal: %u\n \tHex: 0x%08x\n", item_three, item_three);

    lseek(fd, 512, SEEK_SET);
    read(fd, &item_four, sizeof(uint32_t));
    printf("Item 4:\n");    
    printf("\tDecimal: %d\n \tHex: 0x%08x\n", item_four, item_four);

    lseek(fd, 1234, SEEK_SET);
    read(fd, &item_five, 2048 * sizeof(char));
    printf("Item 5:\n");    
    printf("\t%s\n", item_five);

    lseek(fd, 2573, SEEK_SET);
    item_six s;
    read(fd, &s, sizeof(item_six));
    printf("\tDecimal: %u\n \tHex: 0x%08x\n", s.a, s.a);    
    printf("\tDecimal: %u\n \tHex: 0x%08x\n", s.b, s.b);
    printf("\tDecimal: %d\n \tHex: 0x%08x\n", s.c, s.c);
    printf("\t%s\n", s.d);
    printf("\t%p\n", s.e);
    

     
    lseek(fd, 0xbbb, SEEK_SET);
    read(fd, &item_seven, sizeof(uint64_t));
    printf("Item 7:\n");    
    printf("\tDecimal: %lu\n \tHex: 0x%08lx\n", item_seven, item_seven);   
 
    lseek(fd, -100, SEEK_END);
    read(fd, &item_eight, sizeof(uint32_t));
    printf("Item 8:\n");    
    printf("\tDecimal: %u\n \tHex: 0x%08x\n", item_eight, item_eight);  

    lseek(fd, 16, SEEK_END);
    write(fd, &item_nine, 5 * sizeof(char));

}
