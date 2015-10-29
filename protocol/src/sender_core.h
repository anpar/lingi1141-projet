#ifndef __SENDER_CORE_H_
#define __SENDER_CORE_H_

#include <sys/select.h> /* select */
#include <unistd.h>     /* STDIN_FILENO, STDOUT_FILENO */
#include <stdio.h>      /* perror, fprintf */
#include <fcntl.h>      /*open*/
#include <string.h>     /* memset */
#include <signal.h>     /* signal */
#include <time.h>       /* timer_t, ... */
#include <stdbool.h>    /* true, false */

#include "packet_interface.h"

#define SENDER_WINDOW_SIZE 32      // Taille de la fenêtre de l'émetteur
#define MAX_PKT_SIZE 520    // Taille maximum d'un paquet de donnée

/*
 * Structure représentant un paquet envoyé. Elle contient:
 * - les données de ce paquet dans pkt_buf en network byte-order.
 * - un booléan ack qui vauttrue si l'ack correspondant a été reçu, false sinon.
 * - un champ indiquant la taille utilisée de pkt_buf.
 * - un timer.
 */
struct pkt_sent {
  char pkt_buf[MAX_PKT_SIZE];
  bool ack;
  int data_size;
  timer_t timerid;
};

/* Fenêtre de l'émetteur */
struct pkt_sent swin[SENDER_WINDOW_SIZE];
/* Entier enregistrant la valeur du dernier ACK reçu */
int lastack;
/* Entier enregistrant le nombre de places libres dans la fenêtre d'envoi */
int swin_free_space;
/* Entier enregistrant le nombre de places libres dans la fenêtre de réception */
int rwin_free_space;
/* Numéros de séquence des paquets */
int seqnum;
int socket_number;
/* Permet d'empêcher le sender de continuer à essayer de lire sur stdin une
fois que EOF a été atteint sur celui-ci */
bool eof_reached;

/*
 * Cette fonction lit des données dans filename (ou sur stdin si filename
 * est null) et les envoie sur le socket. Elle réceptionne également les ack
 * ou nack et agit en conséquence.
 */
void sender(const int sfd, char * filename);

/*
 * Initialise la fenêtre d'envoi.
 */
void init_swindow();

/*
 * Envoie les données sur le socket. Retourne true si tout s'est bien passé,
 * false dans le cas contraire.
 */
bool send_data(int sfd, char * buf, int size);

/*
 * Renvoie les données sur le socket lorsqu'un timer expire.
 */
void resend_data();

/*
 * Vérifie la validité d'un seqnum reçu dans (n)ack.
 */
bool in_swindow(int seqnum_pkt);

/*
 * Vérifie si toute la fenêtre a été ack.
 */
bool is_window_acknowledged();

/*
 * Décale la fenêtre d'envoi.
 */
void shift_swindow(int k);

#endif
