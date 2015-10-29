#ifndef __RECEIVER_CORE_H_
#define __RECEIVER_CORE_H_

#include <stdio.h>      /* perror, STDOUT_FILENO, fprintf */
#include <unistd.h>		/* close, read, write */
#include <fcntl.h>		/* open */
#include <stdlib.h>		/* malloc, free */
#include <sys/select.h> /* select */
#include <string.h>     /* memcpy */
#include <zlib.h> 		/* crc32 */
#include <stdbool.h> 	/* true, false */

#include "packet_interface.h"

#define MAX_PKT_SIZE 520

struct rwindow {
	pkt_t * buffer[MAX_WINDOW_SIZE];
	int last_in_seq;
	uint8_t free_space;
};

typedef struct rwindow r_win;

/* Loop reading a socket and writing on a socket.
 * This function decode the data read on the socket.
 * @sfd: The socket file descriptor. It is both bound and connected.
 * @filename: if != NULL, the file where data received must be stored.
 * @return: as soon as PTYPE_DATA with length = 0 is received
 */
void receiver(const int sfd, char * filename);

/*
 * Initialise la fenêtre de réception.
 */
r_win * init_rwindow();

/*
 * Libère l'espace alloué pour la fenêtre
 * de réception.
 */
void free_rwindow(r_win * rwin);

/*
 * Décale la fenêtre et affiche sur out_fd les élements
 * en séquences dans la fenêtre (s'il y en a). Returne true
 * si le transfert est terminé, false dans le cas contraire.
 */
bool shift_rwindow(r_win * rwin, int out_fd);

/*
 * Retourne true si le numéro de séquence
 * rentre dans la fenre, false sinon.
 */
bool in_rwindow(r_win * rwin, uint8_t seqnum);

/*
 * Ajoute un paquet dans la fenêtre de réception
 */
void add_in_rwindow(pkt_t * d_pkt, r_win * rwin);

/*
 * Construit un ack pour un paquet reçu
 */
void build_ack(char ack[8], r_win * rwin);

/*
 * Construit un nack pour un paquet reçu
 */
void build_nack(pkt_t * pkt, char nack[8], r_win * rwin);

/*
 * Permet d'écrire les ack et nack sur le socket.
 * Renvoie true si tout s'est bien passé, false sinon.
 */
bool send_ack_or_nack(int sfd, char * buf, int size);

#endif
