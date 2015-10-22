#include "read_write_loop.h"
#include "packet_implem.c"

#include <sys/select.h> /* select */
#include <unistd.h>     /* STDIN_FILENO, STDOUT_FILENO */
#include <stdio.h>      /* perror, fprintf */
#include <fcntl.h>      /*open*/
#include <stdlib.h>
#include <signal.h>
#include <time.h>

#define BUF_SIZE 512 // nombre de bytes lus sur stdin ou dans le fichier
#define WINDOW_SIZE 32 // taille de la fenêtre de l'émetteur


struct window windowTab[WINDOW_SIZE]; // Fenêtre de l'émetteur
int lastack = 0; // Entier enregistrant la valeur du dernier ACK reçu
int window = 32; // Entier enregistrant le nombre de places libres dans la fenêtre
int seqnum = 0; // Numéros de séquence des paquets
int socket;


// On enregistre les valeurs nécessaires pour les timers
struct timespec value;
struct timespec interval;
struct itimerspec itimerspec;


// Permet de mettre toutes les places disponibles dans le buffer
void initWindow()
{
  int j = 0;
  while(j < 32)
  {
    windowTab[j].ack = 0;
    j++;
  }
  value.tv_sec = 3;
  value.tv_nsec = 0;

  interval.tv_sec = 0;
  interval.tv_nsec = 0;

  itimerspec.it_interval = interval;
  itimerspec.it_value = value;

}



int write_on_socket(ssize_t read_on_stdin, char * buf, int sfd)
{
  return 0;
}

//Permet de renvoyer un élément lorsque son timer est tombé à zéro
void hdl(int sig)
{
  struct itimerspec timerspec;
  int j = 0;
  while(j < 32)
  {
    if(timer_gettime(windowTab[j].timerid, &timerspec)!=0)
    {
      perror("timer_gettime()");
      return;
    }
    if(timerspec.it_value.tv_sec==0)
    {
      /* On écrit sur le socket */
      ssize_t ret = write (socket, windowTab[j].pkt_buf, 520);
      if (ret == -1) {
        perror("write()");
        return;
      }
      if(timer_settime(windowTab[j].timerid,0,&itimerspec,NULL)!=0) // RELANCER TIMER
      {
        perror("timer_settime()");
        return;
      }
      return;
    }
    j++;
  }

}

/*
* Loop reading a socket and printing to stdout,
* while reading stdin and writing to the socket
* @sfd: The socket file descriptor. It is both bound and connected.
* @return: as soon as stdin signals EOF
*/
void read_write_loop(int sfd, char * filename) {
  /* Declare variables */
  //struct timeval timeout;
  socket = sfd;
  int retval;
  fd_set readfds, writefds;
  char buf[BUF_SIZE]; // Permet de lire les données
  int fileDesc; // Permet d'enregistrer si c'est stdin ou le fichier


  signal(SIGALRM, hdl); // Lie le signal à la fonction hdl()

  initWindow();

  /* Initialize variables */
  //timeout.tv_sec = 60;
  //timeout.tv_usec = 0;

  while (1) {
    if(filename == NULL)
    {
      fileDesc = STDIN_FILENO;
    }
    else
    {
      fileDesc = open(filename,O_RDONLY);
    }
    FD_SET(fileDesc, &readfds);     // We can read on stdin
    FD_SET(sfd, &writefds);             // We can write on the socket
    FD_SET(sfd, &readfds);              // We can read on the socket

    retval = select(sfd+1, &readfds, &writefds, NULL, NULL);

    if (retval == -1) {
      perror("select()");
      return;
    } else if (!retval) {
      fprintf(stderr, "select() : timeout expires.\n");
      return;
    } else {


      /* Characacters become available for reading on the socket, we have
      to analyse it (ACK or NACK) */
      if (FD_ISSET(sfd, &readfds)) {
        fprintf(stderr, "Reading on the socket and writing on stdout ");
        ssize_t read_on_socket = read(sfd, buf, 8);
        if (read_on_socket == -1) {
          perror("read()");
          return;
        } else if (read_on_socket == 0) {
          fprintf(stderr, "(EOF).\n");
          return;
        }

        fprintf(stderr, "(%d bytes).\n", (int) read_on_socket);

        /* PRENDRE LES MESURES EN FONCTION DE CE QUI EST LU */
        pkt_t * pkt_temp = pkt_new();
        uint8_t seqnum_pkt;
        if(pkt_decode(buf, 8, pkt_temp) != PKT_OK)
        {
          perror("pkt_decode()");
          return;
        }
        /* Si on reçoit un ack (attention:cumulatif), on modifie les données */
        if(pkt_get_type(pkt_temp) == PTYPE_ACK)
        {
          seqnum_pkt = pkt_get_seqnum(pkt_temp);
          if(seqnum_pkt > lastack && seqnum_pkt < lastack + 32)
          {
            int k = seqnum_pkt - lastack; // Nombre d'éléments cumulés reçus
            while (k >= 0) // On vide les éléments qui ont été reçus
            {
              windowTab[k].ack = 0;
              timer_delete(*windowTab[k].timerid);
              window ++;
              k--;
            }
            k = seqnum_pkt - lastack;
            int j = 0;
            while(j < (32-k)) // On slide la fenêtre en deux boucles while
            {
              windowTab[j] = windowTab[k+j];
              j++;
            }
            while(j < 32) // On libère plusieurs cases au bout du tableau
            {
              windowTab[j].ack = 0;
              timer_delete(*windowTab[j].timerid);
            }
            lastack = seqnum_pkt;
          }
        }
        else if(pkt_get_type(pkt_temp) == PTYPE_NACK)
        {
          int i = pkt_get_seqnum(pkt_temp) - lastack;
          windowTab[i].ack = -1;

          /* On écrit sur le socket */
          ssize_t ret = write (sfd, windowTab[i].pkt_buf, 520);
          if (ret == -1) {
            perror("write()");
            return;
          }
          if(timer_settime(windowTab[i].timerid,0,&itimerspec,NULL)!=0) // RELANCER TIMER
          {
            perror("timer_settime()");
            return;
          }
        }
        else
        {
          perror("Nor an ACK nor a NACK");
          return;
        }
        pkt_del(pkt_temp);

      }


      /* Characacters become available for reading on stdin or in the file,
      we have to directly write those characacters on the socket. Need a free
      place in the windowtable */
      if (FD_ISSET(fileDesc, &readfds) && FD_ISSET(sfd, &writefds) && window > 0) {
        fprintf(stderr, "Reading on fileDesc and writing on the socket ");
        ssize_t read_on_stdin = read(fileDesc, buf, BUF_SIZE);
        if (read_on_stdin == -1) {
          perror("read()");
          return;
        } else if (read_on_stdin == 0) {
          fprintf(stderr, "(EOF).\n");
          return;
        }

        fprintf(stderr, "(%d bytes).\n", (int) read_on_stdin);

        /* On trouve une case du tablea libre, on initialise ses attributs
        ack et timer*/
        int i;
        while(windowTab[i].ack != 0)
        {
          i++;
        }
        windowTab[i].ack = -1;
        if(timer_create(CLOCK_REALTIME,NULL,windowTab[i].timerid)!=0)
        {
          perror("timer_create()");
          return;
        }

        pkt_t * pkt_temp = pkt_new();
        pkt_set_type(pkt_temp, PTYPE_DATA);
        pkt_set_window(pkt_temp, window);
        pkt_set_seqnum(pkt_temp, seqnum);
        pkt_set_length(pkt_temp, read_on_stdin);

        /* On encode en pkt pour obtenir le CRC et le mettre dans un buffer */
        if(pkt_encode(pkt_temp, windowTab[i].pkt_buf, &read_on_stdin) != PKT_OK)
        {
          perror("pkt_encode()");
          return;
        }
        window --; // Il y a une place de libre de moins

        /* On écrit sur le socket */
        ssize_t written_on_socket = 0;
        while (written_on_socket != (read_on_stdin + 8)) {
          ssize_t ret = write (sfd, windowTab[i].pkt_buf, (read_on_stdin + 8) - written_on_socket);
          if (ret == -1) {
            perror("write()");
            return;
          }
          written_on_socket += ret;
        }
        if(timer_settime(windowTab[i].timerid,0,&itimerspec,NULL)!=0) //LANCER TIMER
        {
          perror("timer_settime()");
          return;
        }
        pkt_del(pkt_temp);
        seqnum ++;
      }
    }
  }
}
