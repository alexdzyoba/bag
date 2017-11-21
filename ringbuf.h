#ifndef __BAG_H
#define __BAG_H

#include <stdlib.h>

struct ringbuf_item {
    char *buf;  // Character buffer
    size_t len; // Length of data read into buffer
    size_t pos; // Current position in buf if write was incomplete
};

struct ringbuf {
    struct ringbuf_item **items;
    int n;          // Items in buffer
    size_t bufsize; // Size of buffer in each ringbuf item
    int head;
    int tail;
};

struct ringbuf *ringbuf_alloc(int nitems, size_t bufsize);
void ringbuf_free(struct ringbuf *ringbuf);

int ringbuf_read_fd(int fd, struct ringbuf *ringbuf);
int ringbuf_write_fd(int fd, struct ringbuf *ringbuf);

#endif // __BAG_H
