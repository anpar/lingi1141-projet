#include "real_address.h"

#include <netdb.h>
#include <netinet/in.h>
#include <string.h>
#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <stdlib.h>

const char * real_address(const char * address, struct sockaddr_in6 * rval) {
    struct addrinfo hints;
    struct addrinfo * result;
    int s;

    if(rval == NULL)
        return("rval can't be NULL.\n");

     /* Initialize hints */
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_INET6;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_protocol = 17;
    hints.ai_flags = 0;
    hints.ai_canonname = NULL;
    hints.ai_addr = NULL;
    hints.ai_next = NULL;

    s = getaddrinfo(address, NULL, &hints, &result);
    if(s != 0)
            return(gai_strerror(s));

    struct sockaddr_in6 * saddr = (struct sockaddr_in6 *) result->ai_addr;
    memcpy(rval, saddr, result->ai_addrlen);

    /* Free result addrinfo */
    freeaddrinfo(result);

	return(NULL);
}
