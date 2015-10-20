#include "read_loop.h"

#include <stdio.h>      /* perror, STDOUT_FILENO, fprintf */
#include <stdbool.h>    /* true, false */
#include <unistd.h>		/* close */
#include <fcntl.h>		/* open */
#include <stdlib.h>		/* malloc, free */
#include <sys/select.h> /* select */

#include "packet_interface.h"

#define WIN_SIZE 31
#define MAX_PKT_SIZE 520

struct window {
	pkt_t * buffer[31];
	int last_in_seq;
	int free_space;
};

typedef struct window win;

win * init_window()
{
	win *w = (win *) malloc(sizeof(win));
	if(w == NULL)
		return NULL;
		
	w->last_in_seq = 0;
	w->free_space = WIN_SIZE;
	
	return w;
}

void free_window(win * rwin)
{
	free(rwin);
	rwin = NULL;
}

void shift_window(win * rwin, int out_fd) 
{
	
}

bool in_window(win * rwin, uint8_t seqnum) 
{
	int max = (rwin->last_in_seq + WIN_SIZE) % 256;
	int min = rwin->last_in_seq;
	
	/*
	 * La fenêtre ressemble à ça (valeurs
	 * possibles = *) :
	 *      max          min
	 * ******-------------*****
	 */
	if((rwin->last_in_seq + WIN_SIZE) > 256) {
		return (seqnum > min) || (seqnum <= max);
	} 
	/* La fenêtre ressemble à ça (valeurs
	 * possibles = *) :
	 * 		 min      max
	 * -------*********--------
	 */
	else {
		return (seqnum > min) && (seqnum <= max); 
	}
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
		fd = open(filename, O_WRONLY|O_CREAT|O_TRUNC);
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
				/* On lit les données sur le socket */
				ssize_t read_on_socket = read(sfd, (void *) buf, MAX_PKT_SIZE);
				if (read_on_socket == -1) {
                    perror("read()");
                    return;
                }
                
				/* On décode les données reçues */
                pkt_t *d_pkt = pkt_new();
                pkt_status_code c = pkt_decode((const char *) buf, (const size_t) read_on_socket, d_pkt);
                /* Si le paquet reçu est valide et que le numéro de séquence
                   rentre dans le fenêtre de réception, on le traite */
                if(c == PKT_OK && in_window(rwin, pkt_get_seqnum(d_pkt)))
                {
					
				}
				
				/* Sinon on ne fait rien, on l'ignore */
			}
			
			/* On peut écrire sur out_fd */
			if(FD_ISSET(out_fd, &writefds))
			{
				shift_window(rwin, out_fd);
			}
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
