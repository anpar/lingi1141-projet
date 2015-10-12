#include "read_write_loop.h"

#include <poll.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

#define BUF_SIZE 1024

/*
 * Loop reading a socket and printing to stdout,
 * while reading stdin and writing to the socket
 * @sfd: The socket file descriptor. It is both bound and connected.
 * @return: as soon as stdin signals EOF
 */

void read_write_loop(int sfd) {

        fprintf(stderr,"FONCTION read_write_loop.\n");

        /*
         * struct pollfd is defined as :
         *
         * struct pollfd {
         *      int   fd;       file descriptor
         *      short events;   requested events
         *      short revents;  returned events
         * };
         */

        struct pollfd fds[3];
        int timeout_msecs = 500;
        int eof_reached = 0;
        int ret, i;
        char * buf = (char *) malloc(sizeof(char));

        /* We can read and write on the socket */
        fds[0].fd = sfd;
        fds[0].events = POLLIN | POLLOUT;
        /* We can read on stdin */
        fds[1].fd = STDIN_FILENO;
        fds[1].events = POLLIN;
        /* We can write on stdout */
        fds[2].fd = STDOUT_FILENO;
        fds[2].events = POLLOUT;

        while(!eof_reached) {
                ret = poll(fds, 3, timeout_msecs);
                if(ret > 0) {
        	        for(i = 0; i < 3; i++) {
                                /* Un fd est disponible en lecture */
            	                if(fds[i].revents & POLLIN) {
                                        fprintf(stderr, "fd[%d] disponible en lecture.\n", i);
                                        /* Ce fd est le socket */
                                        if(i == 0) {
                                                FILE * f  = fdopen(fds[2].fd, "a");
                                                while(read(sfd, (void *) buf, 1)) {
                                                        fprintf(stderr, "Reading from the socket and writing on stdout.\n");
                                                        fwrite(buf, 1, 1, f);
                                                }
                                                fprintf(stderr, "Stop writing on stdout.\n");

                                                fclose(f);
                                        }
                                        /* Ce fd est stdin */
                                        if(i == 1) {
                                                FILE * f = fdopen(fds[1].fd, "r");
                                                while(fread(buf, 1, 1, f) && !feof(f)) {
                                                        fprintf(stderr, "Reading from stdin and writing on the socket.\n");
                                                        write(sfd, buf, 1);
                                                }
                                                fprintf(stderr, "Stop writing on the socket.\n");

                                                eof_reached = 1;
                                                fclose(f);
                                        }
                                }

                                /* Un fd est disponible en écriture */
                                if(fds[i].revents & POLLWRNORM) {
                                        fprintf(stderr, "fd[%d] disponible en écriture.\n", i);
                                        /* Ce fd est le socket */
                                        if(i == 0) {
                                                FILE * f = fdopen(fds[1].fd, "r");
                                                while(fread(buf, 1, 1, f) && !feof(f)) {
                                                        fprintf(stderr, "Reading from stdin and writing on socket.\n");
                                                        write(sfd, buf, 1);
                                                }
                                                fprintf(stderr, "Stop writing on socket.\n");

                                                eof_reached = 1;
                                                fclose(f);
                                        }
                                        /* Ce fd est stdout */
                                        if(i == 2) {
                                                FILE * f = fdopen(fds[2].fd, "a");
                                                while(read(sfd, (void *) buf, 1)) {
                                                        fprintf(stderr, "Reading from socket and writing on stdout.\n");
                                                        fwrite(buf, 1, 1, f);
                                                }
                                                fprintf(stderr, "Stop writing on stdout.\n");

                                                fclose(f);
                                        }
                                }
        	        }
                } else if(ret == 0) {
                        fprintf(stderr, "poll() failed : the call timed out and no file descriptors were ready.\n");
                } else {
                        fprintf(stderr, "poll() failed : %s.\n", strerror(errno));
                }
        }

        fprintf(stderr, "Leaving read_write_loop.\n");

        free(buf);
}
