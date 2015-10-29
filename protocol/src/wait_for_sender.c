#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <string.h>

#include "wait_for_sender.h"

#define BUF_SIZE 1024

int wait_for_sender(int sfd){
    char buf[BUF_SIZE];
    int nread;
    struct sockaddr_in6 peer_addr;
	peer_addr.sin6_family = AF_INET6;
    socklen_t peer_addr_len = sizeof(struct sockaddr_in6);

    nread = recvfrom(sfd, buf, BUF_SIZE, MSG_PEEK, (struct sockaddr *) &peer_addr, &peer_addr_len);
    if(nread >= 0) {
        if(connect(sfd, (const struct sockaddr *) &peer_addr, peer_addr_len) == 0) {
            return(0);
        }

        return(-1);
    } else {
        perror("recvfrom()");
        return(-1);
    }
}
