/* sockqueue.h */

#ifndef __CS361_SOCKQUEUE__
#define __CS361_SOCKQUEUE__

#include <sys/socket.h>
#include <stdint.h>

/*
    This is the main data structure for the queue. It contains pointers
    to the first and last element in the queue.
*/
struct sockqueue {
    struct sq_elem *head;
    struct sq_elem *tail;
};

/* 
    Queue elements are linked together in a doubly-linked list. There
    are next and previous pointers, and the integer value.
*/
struct sq_elem {
    struct sq_elem *next;
    struct sq_elem *prev;
    struct sockaddr_in *value;
};

void init_sock_queue(struct sockqueue *q);
int sock_enqueue(struct sockqueue *q, struct sockaddr_in*);
struct sockaddr_in *sock_dequeue(struct sockqueue *q);
int sock_isempty(struct sockqueue *q);

struct sock_busy_list {
    struct sock_busy_elem *first;
};

struct sock_busy_elem {
    struct sock_busy_elem *next;
    struct sockaddr_in *elem;
};

void sock_busy_init(struct sock_busy_list *l);
int sock_busy_add(struct sock_busy_list *l, struct sockaddr_in *s);
struct sockaddr_in *sock_busy_remove(struct sock_busy_list *l, uint16_t port, uint32_t addr);
int sock_busy_isempty(struct sock_busy_list *l);

#endif
