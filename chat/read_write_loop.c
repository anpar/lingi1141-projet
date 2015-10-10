#include "read_write_loop.h"

#include <poll.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

void read_write_loop(int sfd) {

    fprintf(stderr,"FONCTION read_write_loop \n");
    
    struct pollfd fds[3];
    int timeout_msecs = 500;
    int eof_reached = 0;
    int ret, i;
    char * buf = (char *) malloc(sizeof(char));
    
    fds[0].fd = sfd;
    fds[1].fd = 0;
    fds[2].fd = 1;
    fds[0].events = POLLRDNORM | POLLWRNORM;
    fds[1].events = POLLRDNORM;
    fds[2].events = POLLWRNORM;
    
    while(!eof_reached) {
        ret = poll(fds, 3, timeout_msecs);
        if(ret > 0) {
        	for(i = 0; i < 3; i++) {
                /* Un fd est disponible en lecture */
            	if(fds[i].revents & POLLRDNORM) {
                    /* Ce fd est le socket */ 
                    if(i == 0) {
                        while(read(sfd, (void *) buf, 1)) {
                            fwrite(buf, 1, 1, (FILE *) &(fds[2].fd));
                        }
                    }
                    /* Ce fd est stdin */
                    if(i == 1) {
                        while(fread(buf, 1, 1, (FILE *) &(fds[1].fd))) {
                            write(sfd, buf, 1);
                        }
                        
                        eof_reached = 1;
                    }
                }
                /* Un fd est disponible en écriture */
                if(fds[i].revents & POLLWRNORM) {
                    /* Ce fd est le socket */
                    if(i == 0) {
                        while(fread(buf, 1, 1, (FILE *) &(fds[0].fd))) {
                            write(sfd, buf, 1);
                        }
                    }
                    /* Ce fd est stdout */
                    if(i == 2) {
                        while(read(sfd, (void *) buf, 1)) {
                            fwrite(buf, 1, 1,(FILE *) &(fds[2].fd));
                        }
                    }
                }
        	}
        }
    }
    
    free(buf);
}
