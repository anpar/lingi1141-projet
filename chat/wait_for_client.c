#include "wait_for_client.h"

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <string.h>

#define BUF_SIZE 1024

/*
 * Block the caller until a message is received on sfd,
 * and connect the socket to the source addresse of the received message.
 * @sfd: a file descriptor to a bound socket but not yet connected
 * @return: 0 in case of success, -1 otherwise
 * @POST: This call is idempotent, it does not 'consume' the data of the message,
 * and could be repeated several times blocking only at the first call.
 */

int wait_for_client(int sfd){
    char buf[BUF_SIZE];
    int nread;
    struct sockaddr_in6 peer_addr;
	peer_addr.sin6_family = AF_INET6;
    socklen_t peer_addr_len = sizeof(struct sockaddr_in6);

    fprintf(stderr, "Server waiting the first message from the client...\n");
    nread = recvfrom(sfd, buf, BUF_SIZE, MSG_PEEK, (struct sockaddr *) &peer_addr, &peer_addr_len);
    if(nread >= 0) {
        if(connect(sfd, (const struct sockaddr *) &peer_addr, peer_addr_len) == 0) {
            fprintf(stderr, "Server is connected to the client.\n");
            return(0);
        }

        return(-1);
    } else {
        perror("recvfrom()");
        return(-1);
    }
}
