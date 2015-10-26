#include <stdio.h>      /* perror, STDOUT_FILENO, fprintf */
#include <stdbool.h>    /* true, false */
#include <unistd.h>		/* close, read, write */
#include <fcntl.h>		/* open */
#include <stdlib.h>		/* malloc, free */
#include <sys/select.h> /* select */

#include "read_loop.h"

/*
 * Une vraie opération modulo (au sens mathématique).
 * L'opérateur % n'étant pas un vrai un modulo (mais plutôt
 * un calcul de reste, ce qui n'est pas la même chose!)
 */
 int mod(int a, int b)
 {
    int r = a % b;
    return r < 0 ? r + b : r;
 }

/*
 * Initialise la fenêtre de réception.
 */
win * init_window()
{
	win *w = (win *) malloc(sizeof(win));
	if(w == NULL)
		return NULL;

	w->last_in_seq = -1;
	w->free_space = WIN_SIZE;

	int i;
	for(i = 0; i < WIN_SIZE; i++) {
		w->buffer[i] = NULL;
	}

	return w;
}

/*
 * Libère l'espace alloué pour la fenêtre
 * de réception.
 */
void free_window(win * rwin)
{
	free(rwin);
	rwin = NULL;
}

/*
 * Décale la fenêtre et affiche sur out_fd les élements
 * en séquences dans la fenêtre (s'il y en a). Retourne
 * si le transfert est terminé, false dans le cas contraire.
 */
bool shift_window(win * rwin, int out_fd)
{
    int i;
    for(i = 0; i < WIN_SIZE; i++) {
        if(rwin->buffer[i] != NULL) {
            // En principe toujours vrai
            if(pkt_get_seqnum(rwin->buffer[i]) == ((rwin->last_in_seq + 1) % 256)) {
                rwin->last_in_seq = (rwin->last_in_seq + 1) % 256;
                //fprintf(stderr,"last_in_seq de %d à %d \n", (int) rwin->last_in_seq - 1, (int) rwin->last_in_seq);

                ssize_t written_on_out = 0;
                while (written_on_out != pkt_get_length(rwin->buffer[i])) {
                    ssize_t ret = write (out_fd, (void *) pkt_get_payload(rwin->buffer[i]), pkt_get_length(rwin->buffer[i])-written_on_out);
                    if (ret == -1) {
                        perror("write()");
                    }

                    written_on_out += ret;
                }
                //fprintf(stderr, "printed on_out %d bytes \n", (int) written_on_out);

                pkt_del(rwin->buffer[i]);
                rwin->buffer[i] = NULL;
                rwin->free_space++;

                /*
                    Si on arrive ici, c'est que la longueur du paquet reçu vaut 0,
                    on est donc à la fin du transfert.
                */
                if(written_on_out == 0) {
                    return true;
                }
            }
        } else {
			break;
        }
    }

	/* Il faut ensuite décaler chaque élément de la fenêtre de i
	place vers la gauche */
	int j;
	for(j = i+1; j < WIN_SIZE && i != 0; j++) {
		rwin->buffer[j-i] = rwin->buffer[j];
	}

    return false;
}

/*
 * Retourne true si le numéro de séquence
 * rentre dans la fenre, false sinon.
 */
bool in_window(win * rwin, uint8_t seqnum)
{
	int max = (rwin->last_in_seq + WIN_SIZE) % 256;
	int min = rwin->last_in_seq;

    //fprintf(stderr,"max %d min %d seqnum %d last_in_seq %d \n", max, min, (int) seqnum, (int) rwin->last_in_seq);

	/*
	 * La fenêtre ressemble à ça :
	 *      max          min
	 * ------]------------|-----
	 */
	if((rwin->last_in_seq + WIN_SIZE) >= 256) {
		return (seqnum > min) || (seqnum <= max);
	}
	/* La fenêtre ressemble à ça :
	 * 		 min      max
	 * -------|--------]--------
	 */
	else {
		return (seqnum > min) && (seqnum <= max);
	}
}

/*
 * Construit un ack pour un paquet reçu
 */
void build_ack(char ack[4], win * rwin) {
    ack[0] = (PTYPE_ACK << 5) | rwin->free_space;

    // On compte le nombre d'ack en séquence pour trouver le dernier en séquence
    int i = 0;
    while(rwin->buffer[i] != NULL) {
        i++;
    }

    ack[1] = (rwin->last_in_seq + i + 1) % 256;
	ack[2] = 0;
	ack[3] = 0;
}

/*
 * Construit un nack pour un paquet reçu
 */
void build_nack(pkt_t * pkt, char nack[4], win * rwin) {
    nack[0] = (PTYPE_NACK << 5) | rwin->free_space;
    nack[1] = pkt_get_seqnum(pkt);
    nack[2] = 0;
	nack[3] = 0;
}

/*
 * Permet d'écrire les ack et nack sur le socket.
 * Renvoie true si tout s'est bien passé, false sinon.
 */
bool write_on_socket(int sfd, char * buf, int size) {
    ssize_t written_on_socket = 0;
    while (written_on_socket != size) {
        ssize_t ret = write (sfd, (void *) buf, size-written_on_socket);
        if (ret == -1) {
            perror("write()");
            return false;
        }

        written_on_socket += ret;
    }

    return true;
}

void add_in_window(pkt_t * d_pkt, win * rwin) {
	rwin->free_space--;     // Une place de moins dans rwin
	rwin->buffer[mod(pkt_get_seqnum(d_pkt) - (rwin->last_in_seq + 1), 256)] = d_pkt;
}

void read_loop(int sfd, char * filename)
{
	int err;
	int fd = -1;
	bool ff;
	int retval;
    fd_set readfds, writefds;
    char buf[MAX_PKT_SIZE];

	/* Ouverture de filename s'il est spécifié */
	if((ff = (filename != NULL))) {
		fd = open(filename, O_WRONLY|O_CREAT|O_TRUNC, 777);
		if(fd == -1) {
			perror("open()");
			return;
		}
	}

	/* Initialisation de la fenêtre de réception */
	win * rwin = init_window();

	int out_fd;
	if(ff)
		out_fd = fd;				// out_fd = fd si ff == true
	else
		out_fd = STDOUT_FILENO;		// out_fd = stdout sinon

	while(true)
	{
		FD_SET(sfd, &readfds);     	// On peut lire sur le socket
        FD_SET(sfd, &writefds);     // On peut écrire sur le socket
		FD_SET(out_fd, &writefds);	// On peut écrire sur out_fd

		retval = select(sfd+1, &readfds, &writefds, NULL, NULL);
        if (retval == -1) {
            perror("select()");
            return;
        } else if (!retval) {
            fprintf(stderr, "select() : timeout expires.\n");
            return;
        }
        else {
			/* On peut lire (paquet de données) et écrire (ack
			ou nack correspondant) sur le socket */
			if(FD_ISSET(sfd, &readfds) && FD_ISSET(sfd, &writefds))
			{
                fprintf(stderr, "There is something to read on the socket.\n");
				/* On lit les données sur le socket */
				ssize_t read_on_socket = read(sfd, (void *) buf, MAX_PKT_SIZE);
				if (read_on_socket == -1) {
                    perror("read()");
                    return;
                }

				/* On décode les données reçues dans d_pkt */
                pkt_t *d_pkt = pkt_new();
                pkt_status_code c = pkt_decode((const char *) buf, (const size_t) read_on_socket, d_pkt);
                //fprintf(stderr, "status_code %d et get_seqnum() %d et in_window %d \n", c, (int) pkt_get_seqnum(d_pkt), in_window(rwin, pkt_get_seqnum(d_pkt)));
                /* Si le paquet reçu est valide et que le numéro de séquence
                   rentre dans le fenêtre de réception, on le traite */
                if(c == PKT_OK && in_window(rwin, pkt_get_seqnum(d_pkt)))
                {
                    fprintf(stderr, "And this thing is valid and can go in the reception window.\n");
                    /* Si le packet lu est de type PTYPE_DATA... */
                    if(pkt_get_type(d_pkt) == PTYPE_DATA)
                    {
                        fprintf(stderr, "This is a PTYPE_DATA packet.\n");
                        /* ... et qu'il contient plus de 4 bytes. Alors, on
                        le place dans la fenêtre de réception et on envoie
                        un ACK */
                        if(read_on_socket != 4) {
                            fprintf(stderr, "No network overload indicated.\n");
                            char ack[4];
							add_in_window(d_pkt, rwin);
                            build_ack(ack, rwin);
                            if(!write_on_socket(sfd, ack, 4))
                                return;
                        }
                        /* Sinon, le paquet a été coupé à cause de congestion
                        dans le réseau, il faut avertir le sender en envoyant
                        un NACK */
                        else {
                            fprintf(stderr, "Network overload indicated!\n");
                            char nack[4];
                            build_nack(d_pkt, nack, rwin);
                            if(!write_on_socket(sfd, nack, 4))
                                return;
                        }
                    }
				} else {
				    /* Sinon on ne fait rien, on l'ignore */
                    fprintf(stderr, "This thing isn't valid or can't go in the reception window (%d).\n", (int) c);
                }
            }

			/* On peut écrire sur out_fd */
            // if(FD_ISSET(out_fd, &writefds)) {
				if(shift_window(rwin, out_fd)) {
                    fprintf(stderr, "Fin du transfert.\n");
                    break;
               }
			// }
		}
	}

	/* Libération de l'espace alloué à la fenêtre de réception */
	free(rwin);

	/* Femeture du fd de filename s'il a est spécifié */
	if(ff) {
		err = close(fd);
		if(err == -1)
			perror("close()");
	}
}
