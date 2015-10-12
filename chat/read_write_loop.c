#include "read_write_loop.h"

#include <sys/select.h> /* select */
#include <unistd.h>     /* STDIN_FILENO, STDOUT_FILENO */
#include <stdio.h>      /* perror, fprintf */

#define BUF_SIZE 1024

/*
 * Loop reading a socket and printing to stdout,
 * while reading stdin and writing to the socket
 * @sfd: The socket file descriptor. It is both bound and connected.
 * @return: as soon as stdin signals EOF
 */
void read_write_loop(int sfd) {
    /* Declare variables */
    struct timeval timeout;
    int retval, eof_reached;
    fd_set readfds, writefds;
    char buf[BUF_SIZE];

    /* Initialize variables */
    timeout.tv_sec = 60;
    timeout.tv_usec = 0;

    eof_reached = 0;

    while (1) {
        FD_SET(STDIN_FILENO, &readfds);     // We can read on stdin
        FD_SET(sfd, &writefds);             // We can write on the socket
        if(!eof_reached) {
            FD_SET(STDOUT_FILENO, &writefds);  // We can write on stdout
            FD_SET(sfd, &readfds);             // We can read on the socket
        }

        retval = select(sfd+1, &readfds, &writefds, NULL, &timeout);

        if (retval == -1) {
            perror("select()");
            return;
        } else if (!retval) {
            fprintf(stderr, "select() : timeout expires.\n");
            return;
        } else {
            /* Characacters become available for reading on stdin, we have
            to directly write those characacters on the socket */
            if (FD_ISSET(STDIN_FILENO, &readfds) && FD_ISSET(sfd, &writefds)) {
                fprintf(stderr, "Reading on stdin and writing on the socket.\n");
                ssize_t read_on_stdin = read(STDIN_FILENO, buf, BUF_SIZE);
                if (read_on_stdin == -1) {
                    perror("read()");
                    return;
                } else if (read_on_stdin == EOF) {
                    fprintf(stderr, "read_on_stdin == EOF");
                    return;
                }

                ssize_t written_on_socket = 0;
                while (written_on_socket != read_on_stdin) {
                    ssize_t ret = write (sfd, buf, read_on_stdin);
                    if (ret == -1) {
                        perror("write()");
                        return;
                    }

                    written_on_socket += ret;
                }
            }

            /* Characacters become available for reading on the socket, we have
            to directly write those characacters on stdout */
            if (FD_ISSET(sfd, &readfds) && FD_ISSET(STDOUT_FILENO, &writefds) && !eof_reached) {
                fprintf(stderr, "Reading on the socket and writing on stdout.\n");
                ssize_t read_on_socket = read(sfd, buf, BUF_SIZE);
                if (read_on_socket == -1) {
                    perror("read()");
                    return;
                } else if (read_on_socket == EOF) {
                    eof_reached = 1;
                    fprintf(stderr, "read_on_socket == EOF");
                }

                ssize_t written_on_stdout = 0;
                while (written_on_stdout != read_on_socket) {
                    ssize_t ret = write(STDOUT_FILENO, buf, read_on_socket);
                    if (ret == -1) {
                        perror("write()");
                        return;
                    }

                    written_on_stdout += ret;
                }
            }
        }
    }
}
