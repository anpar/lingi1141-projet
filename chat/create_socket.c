#include "create_socket.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include <string.h>

int create_socket(struct sockaddr_in6 *source_addr, int src_port, struct sockaddr_in6 *dest_addr, int dst_port) {

        fprintf(stderr,"FONCTION create_socket.\n");

        int sockfd = socket(AF_INET6, SOCK_DGRAM, 17);
        
        if(sockfd == -1) {
                fprintf(stderr,"socket() failed.\n");
                return(-1);
        }

        if(source_addr != NULL) { 
                socklen_t addrlen = sizeof(*source_addr); 
                source_addr->sin6_port = src_port;
                if(bind(sockfd, (struct sockaddr *) source_addr, addrlen)  == -1) {
                        fprintf(stderr,"bind() failed : %s.\n", strerror(errno));
                        return(-1);
                }
        } else {
                 fprintf(stderr,"source_addr is null.\n");
        }
      
        if(dest_addr != NULL) {
                socklen_t addrlen = sizeof(*dest_addr);
                dest_addr->sin6_port = dst_port;
                if(connect(sockfd, (struct sockaddr *) dest_addr, addrlen)  == -1) {
                        fprintf(stderr,"connect() failed : %s.\n", strerror(errno));
                        return(-1);
                }
        } else {
                 fprintf(stderr,"dest_addr nulle.\n");
        }

        return(sockfd);
}
