#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <limits.h>
#include <poll.h>
#include <signal.h>
#include <sys/signalfd.h>
#include <fcntl.h>

#include "ringbuf.h"

static struct ringbuf *ringbuf;
static int sigfd;
static bool stop_output = false;
static bool input_hangup = false;
static const size_t bufsize = 16 * LINE_MAX;

static int signalfd_setup()
{
    sigset_t sigmask;
    sigemptyset(&sigmask);
    sigaddset(&sigmask, SIGTERM); // To free ringbuf
    sigaddset(&sigmask, SIGINT); // To free ringbuf
    sigaddset(&sigmask, SIGUSR1); // To pause output
    sigaddset(&sigmask, SIGUSR2); // To continue output

    // Block signals so that they aren't handled
    // according to their default disposition
    if (sigprocmask(SIG_BLOCK, &sigmask, NULL) == -1) {
        perror("sigprocmask");
        return -1;
    }

    // Ignore pipe EOF
    signal(SIGPIPE, SIG_IGN);

    sigfd = signalfd(-1, &sigmask, 0);
    if (sigfd < 0) {
        perror("signalfd");
        return -1;
    }

    return 0;
}

static int signalfd_read(int fd)
{
    struct signalfd_siginfo ssi;
    ssize_t n = read(fd, &ssi, sizeof(struct signalfd_siginfo));
    if (n != sizeof(struct signalfd_siginfo)) {
        perror("read");
        return -1;
    }

    if (ssi.ssi_signo == SIGTERM || ssi.ssi_signo == SIGINT) {
        fprintf(stderr, "Exiting\n");
        return 1;
    } else if (ssi.ssi_signo == SIGUSR1) {
        stop_output = true;
        fprintf(stderr, "Pausing output\n");
    } else if (ssi.ssi_signo == SIGUSR2) {
        stop_output = false;
        fprintf(stderr, "Resuming output\n");
    }
    return 0;
}

static int poll_fds()
{
    int infd = 0; // open("input", O_RDONLY); // stdin
    int outfd = 1; //open("output", O_WRONLY); // stdout

    struct pollfd fds[3];
    fds[0].fd = sigfd; // signals
    fds[0].events = POLLIN;
    fds[1].fd = infd;
    fds[1].events = POLLIN;
    fds[2].fd = outfd;
    fds[2].events = POLLOUT;

    int nfds = 3;

    while (1) {
        int ret = poll(fds, nfds , -1);
        switch (ret) {
            case -1:
                perror("poll");
                return -1;
                break;

            case 0:
                fprintf(stderr, "Poll timeout\n");
                break;

            default:
                if (fds[0].revents & POLLIN) {
                    int rc = signalfd_read(fds[0].fd);

                    if (rc == 0) {
                        if (stop_output == false) {
                            // If output resumed, restart polling it
                            fds[2].events = POLLOUT;
                        }
                    } else {
                        return rc;
                    }
                }

                if (fds[1].revents & POLLIN) {
                    bool ringbuf_have_space = (ringbuf->head != ringbuf->tail);
                    if (ringbuf_have_space) {
                        if (ringbuf_read_fd(fds[1].fd, ringbuf)) {
                            return -1;
                        }
                        // Got input, start polling stdout
                        fds[2].events = POLLOUT;
                    } else if (!input_hangup) {
                        // No space to input, stop polling input
                        fds[1].events = 0;
                    }
                }

                if (fds[2].revents & POLLOUT) {
                    int ringbuf_next = (ringbuf->head + 1) % ringbuf->n;
                    bool ringbuf_have_data = (ringbuf_next != ringbuf->tail);

                    if (!stop_output && ringbuf_have_data) {
                        if (ringbuf_write_fd(fds[2].fd, ringbuf)) {
                            return -1;
                        }
                        // Made space in ringbuf, start polling input
                        fds[1].events = POLLIN;
                    } else if (input_hangup) {
                        fprintf(stderr, "No more input, exiting..\n");
                        return 0;
                    } else {
                        // No data to output, stop polling out
                        fds[2].events = 0;
                    }
                }

                if (fds[1].revents & POLLHUP) {
                    // Input fd was closed and we have to finish reading last
                    // piece of data that might be missed in case of binary data
                    struct ringbuf_item *last = ringbuf->items[ringbuf->tail];
                    if (last->pos) {
                        last->len = last->pos;
                        last->pos = 0;
                        ringbuf->tail = (ringbuf->tail + 1) % ringbuf->n;
                    }

                    input_hangup = true;
                    stop_output = false;
                    fds[2].events = POLLOUT;
                }

                if (fds[2].revents & POLLHUP ||
                    fds[2].revents & POLLERR) {
                    fprintf(stderr, "Output hangup\n");
                    return 0;
                }

                break;
        }
    }
    return -1;
}

int main(int argc, const char *argv[])
{
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <number of lines>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    long nlines;
    errno = 0;
    nlines = strtol(argv[1], NULL, 0);
    if (errno) {
        perror("strtol");
        exit(EXIT_FAILURE);
    }

    if (signalfd_setup()) {
        exit(EXIT_FAILURE);
    }

    ringbuf = ringbuf_alloc(nlines, bufsize);
    if (!ringbuf) {
        exit(EXIT_FAILURE);
    }

    if (poll_fds()) {
        fprintf(stderr, "Poll failed\n");
    }

    ringbuf_free(ringbuf);
    return 0;
}
