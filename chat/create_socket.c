#include "create_socket.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include <string.h>

/*
 * Creates a socket and initialize it
 * @source_addr: if !NULL, the source address that should be bound to this socket
 * @src_port: if >0, the port on which the socket is listening
 * @dest_addr: if !NULL, the destination address to which the socket should send
 * data
 * @return: a file descriptor number representing the socket,
 * or -1 in case of error (explanation will be printed on stderr)
 */

int create_socket(struct sockaddr_in6 *source_addr, int src_port, struct sockaddr_in6 *dest_addr, int dst_port) {
    int sockfd = socket(AF_INET6, SOCK_DGRAM, 0);

    if(sockfd == -1) {
        fprintf(stderr,"socket() failed.\n");
        return(-1);
    }

    /* The client is calling create_socket */
    if(source_addr != NULL) {
        socklen_t addrlen = sizeof(*source_addr);

        if(src_port > 0)
            source_addr->sin6_port = htons(src_port);

        if(bind(sockfd, (struct sockaddr *) source_addr, addrlen)  == -1) {
            fprintf(stderr,"bind() failed : %s.\n", strerror(errno));
            return(-1);
        }
    }

    /* The server is calling create_socket */
    if(dest_addr != NULL) {
        socklen_t addrlen = sizeof(*dest_addr);

        if(dst_port > 0)
            dest_addr->sin6_port = htons(dst_port);

        if(connect(sockfd, (struct sockaddr *) dest_addr, addrlen)  == -1) {
            fprintf(stderr,"connect() failed : %s.\n", strerror(errno));
            return(-1);
        }
    }

    return(sockfd);
}
