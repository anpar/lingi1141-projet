#ifndef __READ_WRITE_LOOP_H_
#define __READ_WRITE_LOOP_H_

#include <sys/select.h> /* select */
#include <unistd.h>     /* STDIN_FILENO, STDOUT_FILENO */
#include <stdio.h>      /* perror, fprintf */
#include <fcntl.h>      /*open*/
#include <stdlib.h>
#include <signal.h>
#include <time.h>
#include <sys/types.h>
#include <stdbool.h>
#include "packet_interface.h"

#define BUF_SIZE 512 // nombre de bytes lus sur stdin ou dans le fichier
#define WINDOW_SIZE 32 // taille de la fenêtre de l'émetteur

/* Structure of an element read by the sender on stdin or in a file
 * this element has 512 bytes + header (4) + CRC (4) (pkt_buf),
 * this element has a transmission timer (timerid)
 * this element has been acknowledged by the receiver (ack = 0) or not (ack = -1)
 */
struct window2 {
  char pkt_buf[520];
  int ack;
  timer_t timerid;
};

//struct timer_t * timers;

struct window2 windowTab[WINDOW_SIZE];  // Fenêtre de l'émetteur
int lastack;                            // Entier enregistrant la valeur du dernier ACK reçu
int windowFree;                         // Entier enregistrant le nombre de places libres dans la fenêtre
int seqnum_maker;                       // Numéros de séquence des paquets
int socketNumber;

/* Loop reading a socket and updating his attributes
 * while reading stdin or a file and writing to the socket
 * @sfd: The socket file descriptor. It is both bound and connected.
 * @filename: If a file is to be written on the socket instead of reading stdin
 * @return: as soon as the last ACK is received after stdin signals EOF
 */
void read_write_loop(const int sfd, char * filename);

void initWindow();

int write_on_socket2(int sfd, char * buf, int size);

//void initTimer(struct timespec valuev, struct timespec intervali, struct itimerspec itimerspeci)

void hdl();

bool seqnum_valid(int seqnum_pkt);

void slideWindowTab(int k);

#endif
