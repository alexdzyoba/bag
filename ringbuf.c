#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#include "ringbuf.h"

struct ringbuf *ringbuf_alloc(int nitems, size_t bufsize)
{
    struct ringbuf *ringbuf;

    ringbuf = malloc(sizeof(*ringbuf));
    if (!ringbuf) {
        perror("malloc");
        return NULL;
    }
    ringbuf->n = nitems;
    ringbuf->bufsize = bufsize;

    ringbuf->items = malloc(ringbuf->n * sizeof(struct ringbuf_item *));
    if (!ringbuf->items) {
        perror("malloc");
        free(ringbuf);
        return NULL;
    }

    for (int i = 0; i < ringbuf->n; i++) {
        ringbuf->items[i] = malloc(sizeof(struct ringbuf_item));
        if (!ringbuf->items[i]) {
            perror("malloc");
            ringbuf_free(ringbuf);
            return NULL;
        }

        ringbuf->items[i]->buf = malloc(bufsize);
        if (!ringbuf->items[i]->buf) {
            perror("malloc");
            ringbuf_free(ringbuf);
            return NULL;
        }
        memset(ringbuf->items[i]->buf, 0, bufsize);
        ringbuf->items[i]->len = bufsize;
        ringbuf->items[i]->pos = 0;
    }

    ringbuf->head = nitems - 1;
    ringbuf->tail = 0;

    return ringbuf;
}

void ringbuf_free(struct ringbuf *ringbuf)
{
    if (ringbuf && ringbuf->items) {
        for (int i = 0; i < ringbuf->n; i++) {
            free(ringbuf->items[i]->buf);
            free(ringbuf->items[i]);
        }
        free(ringbuf->items);
        free(ringbuf);
    }
}

int ringbuf_read_fd(int fd, struct ringbuf *ringbuf)
{
    // Data from stdin is appended to tail
    int cur = ringbuf->tail;
    struct ringbuf_item *cur_item = ringbuf->items[cur];

    ssize_t nread = read(fd,
            cur_item->buf + cur_item->pos,
            cur_item->len - cur_item->pos);

    if (nread < 0) {
        perror("read");
        return -1;
    }

    cur_item->pos += nread;
    // Read complete if there is an end of string
    // or the buffer is filled
    if (strchr(cur_item->buf, '\n') ||
        cur_item->pos == cur_item->len) {
        // Prepare buffer for output
        cur_item->len = cur_item->pos;
        cur_item->pos = 0;

        // Advance tail index
        cur = (cur + 1) % ringbuf->n;
        ringbuf->tail = cur;
    }

    return 0;
}

int ringbuf_write_fd(int fd, struct ringbuf *ringbuf)
{
    // Data to stdout is extracted from next to head
    int cur = (ringbuf->head + 1) % ringbuf->n;
    struct ringbuf_item *cur_item = ringbuf->items[cur];

    ssize_t nwritten = write(fd,
            cur_item->buf + cur_item->pos, 
            cur_item->len - cur_item->pos);

    if (nwritten < 0) {
        perror("write");
        return -1;
    }

    cur_item->pos += nwritten;
    // Write complete
    if (cur_item->pos == cur_item->len) {
        // Reset written buffer
        memset(cur_item->buf, 0, cur_item->len);
        cur_item->pos = 0;
        cur_item->len = ringbuf->bufsize;

        // Advance head index
        ringbuf->head = cur;
    }

    return 0;
}

