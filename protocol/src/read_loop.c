#include "read_loop.h"

#include <stdio.h>      /* perror, STDOUT_FILENO */
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

void shift_window(win * rwin, int fd) 
{
	
}

void read_loop(int sfd, char * filename) 
{
	int err;
	int fd = -1;
	bool ff;
	int retval;
    fd_set readfds, writefds;
	
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
	
	while(true)
	{
		FD_SET(sfd, &readfds);     				// We can read on the socket
        FD_SET(sfd, &writefds);             	// We can write on the socket
        if(ff) 
			FD_SET(fd, &writefds);          	// We can write on filename
        else
			FD_SET(STDOUT_FILENO, &writefds);   // We can write on stdout
			
		retval = select(sfd+1, &readfds, &writefds, NULL, NULL);																							
        
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
