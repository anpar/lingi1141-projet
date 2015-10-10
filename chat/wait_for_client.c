#include "wait_for_client.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h> 

int wait_for_client(int sfd){
  fprintf(stderr,"FONCTION wait_for_client \n");
  char buf[0];
  fprintf(stderr,"FONCTION wait_for_client2 \n");
  int nread;
  fprintf(stderr,"FONCTION wait_for_client3 \n");
  struct sockaddr_in6 peer_addr;
  fprintf(stderr,"FONCTION wait_for_client4 \n");
  socklen_t peer_addr_len;
  fprintf(stderr,"FONCTION wait_for_client5 \n");
  
  nread = recvfrom(sfd, buf, 0, MSG_PEEK, (struct sockaddr *) &peer_addr, &peer_addr_len);
  if(nread >= 0)
  {
    fprintf(stderr,"FONCTION wait_for_client6 \n");
    
    int co = connect(sfd, (const struct sockaddr *) &peer_addr, peer_addr_len);
    if(co == 0)
    {
      fprintf(stderr,"FONCTION wait_for_client7 \n");
      return 0;
    }
    fprintf(stderr,"connect() error \n");
    return -1;
  }
  else
  {
    fprintf(stderr,"recvfrom() error \n");
    return -1;      
  }
}

