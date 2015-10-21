#ifndef __READ_LOOP_H_
#define __READ_LOOP_H_

#include <stdbool.h>
#include "packet_interface.h"

#define WIN_SIZE 31
#define MAX_PKT_SIZE 520

struct window {
	pkt_t * buffer[WIN_SIZE];
	int last_in_seq;
	uint8_t free_space;
};

typedef struct window win;

/* Loop reading a socket and writing on a socket.
 * This function decode the data read on the socket.
 * @sfd: The socket file descriptor. It is both bound and connected.
 * @filename: if != NULL, the file where data received must be stored.
 * @return: as soon as PTYPE_DATA with length = 0 is received
 */
void read_loop(const int sfd, char * filename);

/*
 * Initialise la fenêtre de réception.
 */
win * init_window();

/*
 * Libère l'espace alloué pour la fenêtre
 * de réception.
 */
void free_window(win * rwin);

/*
 * Décale la fenêtre et affiche sur out_fd les élements
 * en séquences dans la fenêtre (s'il y en a).
 */
void shift_window(win * rwin, int out_fd);

/*
 * Retourne true si le numéro de séquence
 * rentre dans la fenre, false sinon.
 */
bool in_window(win * rwin, uint8_t seqnum);

/*
 * Ajoute un paquet dans la fenêtre de réception
 */
void add_in_window(pkt_t * d_pkt, win * rwin);

/*
 * Construit un ack pour un paquet reçu
 * FIX: le CRC doit-il se trouver dans un paquet
 * de type PTYPE_ACK? Pour un PTYPE_NACK c'est explicitement
 * précisé mais quid dans ce cas-ci?
 */
void build_ack(char ack[8], win * rwin);

/*
 * Construit un nack pour un paquet reçu
 */
void build_nack(pkt_t * pkt, char nack[4], win * rwin);

/*
 * Permet d'écrire les ack et nack sur le socket.
 * Renvoie true si tout s'est bien passé, false sinon.
 */
bool write_on_socket(int sfd, char * buf, int size);

#endif
