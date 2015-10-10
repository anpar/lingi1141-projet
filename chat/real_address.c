#include "real_address.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>

const char * real_address(const char * address, struct sockaddr_in6 * rval) {
    
      fprintf(stderr,"FONCTION real_address \n");

    struct addrinfo hints;
    struct addrinfo * result;
    int s;
    
    /* Initialize hints */
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_INET6;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_protocol = 17;
    hints.ai_flags = AI_PASSIVE;
    hints.ai_canonname = NULL;
    hints.ai_addr = NULL;
    hints.ai_next = NULL;
	
    s = getaddrinfo(address, NULL, &hints, &result);
    if(s != 0)
        return(gai_strerror(s));
   
    rval = (struct sockaddr_in6 *) result->ai_addr;
    rval = rval;
    
   	/* Free result addrinfo */
    freeaddrinfo(result);

	return(NULL);
}
