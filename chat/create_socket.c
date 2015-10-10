#include "create_socket.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include <string.h>


int create_socket(struct sockaddr_in6 *source_addr, int src_port,
  struct sockaddr_in6 *dest_addr, int dst_port){


    fprintf(stderr,"FONCTION create_socket \n");

    int sock = socket(AF_INET6, SOCK_DGRAM, 17);
    if(sock == -1)
    {
      fprintf(stderr,"socket() error  \n");
      return -1;
    }
    fprintf(stderr,"socket() OK \n");

    if(source_addr != NULL)
    {
      fprintf(stderr,"source_addr non nulle \n");

      /*On passe par une structure addrinfo*/
      struct addrinfo hints;
      struct addrinfo * result;
      int s;

      /* Initialize hints */
      memset(&hints, 0, sizeof(struct addrinfo));
      hints.ai_family = AF_INET6;
      hints.ai_socktype = SOCK_DGRAM;
      hints.ai_protocol = 17;
      hints.ai_flags = 0;
      hints.ai_canonname = NULL;
      hints.ai_addr = (struct sockaddr *) source_addr;
      hints.ai_addrlen = sizeof(* source_addr);
      hints.ai_next = NULL;

      /*In order to set the port in the addrinfo*/
      char buf[10] = { 0 }; // one extra byte for null
      sprintf(buf, "%d", src_port);

      s = getaddrinfo(NULL, buf, &hints, &result);
      if(s != 0)
      return -1;

      /* We bind the socket with the source address */
      int bi = bind(sock, result->ai_addr, result->ai_addrlen);
      if(bi == -1)
      {
        fprintf(stderr,"bind() error %d \n", errno);
        return -1;
      }
      fprintf(stderr,"bind() OK \n");

      /* Free result addrinfo */
      freeaddrinfo(result);
    }
    else{
      fprintf(stderr,"src_addr nulle -- probablement un client \n");
    }

    if(dest_addr != NULL  )
    {

      fprintf(stderr,"dest_addr non nulle \n");

      /*On passe par une structure addrinfo*/
      struct addrinfo hints;
      struct addrinfo * result;
      int s;

      /* Initialize hints */
      memset(&hints, 0, sizeof(struct addrinfo));
      hints.ai_family = AF_INET6;
      hints.ai_socktype = SOCK_DGRAM;
      hints.ai_protocol = 17;
      hints.ai_flags = 0;
      hints.ai_canonname = NULL;
      hints.ai_addr = (struct sockaddr *) dest_addr;
      hints.ai_addrlen = sizeof(* dest_addr);
      hints.ai_next = NULL;

      /*In order to set the port in the addrinfo*/
      char buf[10] = { 0 }; // one extra byte for null
      sprintf(buf, "%d", dst_port);

      s = getaddrinfo(NULL, buf, &hints, &result);
      if(s != 0)
      return -1;


      /* Connect the socket to the dest_addr*/
      int co = connect(sock, result->ai_addr, result->ai_addrlen);
      if(co == -1)
      {
        fprintf(stderr,"connect() error %d \n", errno);
        return -1;
      }
      fprintf(stderr,"connect() OK \n");

    }
    else{
      fprintf(stderr,"dest_addr nulle -- probablement un serveur \n");
    }

    fprintf(stderr,"tout OK \n");

    return sock;

  }
