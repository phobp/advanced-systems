#include <stdlib.h>
#include <errno.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include "sockqueue.h"


/*
    This function initializes an empty queue. It takes the
    address of an existing struct sockqueue, and sets the head
    and the tail pointers to NULL.
*/
void init_sock_queue(struct sockqueue *q) {
    q->head = NULL;
    q->tail = NULL;
}


/*
    This function takes the address of a struct sockqueue and a pointer to
    a struct sockaddr_in as its parameters. It then allocates memory for a
    new queue element, sets the value of the element to the one passed in, 
    and inserts the new element at the end of the queue.
    Returns 0 on success and -1 when an error has occured.
*/
int sock_enqueue(struct sockqueue *q, struct sockaddr_in *elem) {
    struct sq_elem *new; // pointer to new element
    new = (struct sq_elem *)malloc(sizeof(struct sq_elem)); // try to allocate space
    if (new == NULL)
        return -1;      // if malloc failed, return with error
    new->value = elem;   // set the value of the new element
    new->prev = NULL;   // the new element has no previous element in the queue
    new->next = q->tail;    // the new element will become the new tail,
                            // so its next pointer needs to be the current tail
    if (q->tail != NULL) {  // if the queue is non-empty
        q->tail->prev = new;    // set the current last element's prev pointer
                                // to the new element
    }
    q->tail = new;      // the new element becomes the tail
    if (q->head == NULL) {  // if the queue had been empty
        q->head = q->tail;  // also set head to the new element
    }
    return 0;
}

/*
    This function takes the address of a struct sockqueue, removes its first
    element from the queue if it exists, and returns that element's pointer.
    If the queue is empty, -1 is returned, and errno is set to EFAULT.
*/
struct sockaddr_in *sock_dequeue(struct sockqueue *q) {
    struct sockaddr_in *ret;    // return value; this is needed, because we free the memory
                                // of the queue element before we return
    struct sq_elem *to_remove;  // will point to the queue element we remove
    if (q->head == NULL) {      // if the queue is empty
        errno = EFAULT;         // set errno
        return NULL;
    }
    to_remove = q->head;        // we will remove the first element in the queue
    ret = to_remove->value;     // save the return value
    if (q->head == q->tail) {   // if there is only a single element in the queue
        q->head = NULL;     // mark the queue as empty
        q->tail = NULL;
    }
    else {                  // if there is more than one element
        q->head = to_remove->prev; // the next element becomes the first
        if (q->head != NULL) {  // it really should not be NULL here, but make sure
            q->head->next = NULL; // the new head element does not have a next one
        }
    }
    free(to_remove);    // free the memory
    return ret;     // return the pointer
}

/*
    This function takes the address of a struct sockqueue, and returns 1 is the
    queue is empty and 0 if there are elements in the queue.
*/
int sock_isempty(struct sockqueue *q) {
    return (q->head == NULL);
}

/*
    This functions takes the address of a struct sock_busy_list and initializes it
    to an empty list
*/
void sock_busy_init(struct sock_busy_list *l) {
    l->first = NULL;
}

/*
    This functions takes the address of a struct sock_busy_list and a pointer to
    an existing struct sockaddr_in, and inserts the new element at the beginning
    of the list.
    Returns 0 upon success, and -1 if allocating memory for a new list element failed
*/
int sock_busy_add(struct sock_busy_list *l, struct sockaddr_in *s) {

    struct sock_busy_elem *new; // pointer to new element
    new = (struct sock_busy_elem *)malloc(sizeof(struct sock_busy_elem)); // try to allocate space
    if (new == NULL)
        return -1;      // if malloc failed, return with error
    new->elem = s;	// set the value to be the socket pointer
    new->next = l->first; // use the old first element as the new one's next
    l->first = new;	// make the new element the first one in the list
    return 0;

}

/*
    This functions takes the address of a struct sock_busy_list, a port number, and an
    IPv4 address in binary format as parameters. The list is then traversed, looking for
    a sockaddr_in structure that matches the port number and IP address. If found, that
    element is unlinked from the list, and a pointer to the struct sockaddr_in returned.
    Otherwise, NULL is returned. It is assumed that a given port/address combination
    is contined in the list at most once.
*/
struct sockaddr_in *sock_busy_remove(struct sock_busy_list *l, uint16_t port, uint32_t addr) {

    struct sockaddr_in *res; // pointer to remember result for return
    struct sock_busy_elem *cur, *prev; // pointers to the current and previous elements we are processing
    cur = l->first; // start with the first one
    prev = NULL;  // initially there is no previous element we have visited

    while (cur != NULL) { // while there are elements
        /* check if the port and address match */
        if (cur->elem->sin_port == port && cur->elem->sin_addr.s_addr == addr) {
            res = cur->elem; // if so, we have found our result
            if (prev == NULL) // the match was the first element
                l->first = cur->next; // make the second element the first
            else
                prev->next = cur->next; // otherwise, link the next element to be the next element of the previous one
            free(cur); // free the memory for the list element
            return res;
        }
        prev = cur; // advance current and previous pointers by one element
        cur = cur->next;
    }
    return NULL;
}

/*
    This function takes the address of a struct sock_busy_list, and returns 1 is the
    list is empty and 0 if there are elements in the list.
*/
int sock_busy_isempty(struct sock_busy_list *l) {
    return (l->first == NULL);
}

