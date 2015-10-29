#include "receiver_core.h"

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

r_win * init_rwindow()
{
	r_win *w = (r_win *) malloc(sizeof(r_win));
	if(w == NULL)
		return NULL;

	w->last_in_seq = -1;
	w->free_space = MAX_WINDOW_SIZE;

	int i;
	for(i = 0; i < MAX_WINDOW_SIZE; i++) {
		w->buffer[i] = NULL;
	}

	return w;
}

/*
 * Utilisé pour débuguer le programme. Cette fonction affiche la fenêtre
 * de réception.
 */
void print_rwindow(r_win *rwin) {
    fprintf(stderr, "\nFree space in window : %d. \t Last # acknwoledged : %d.\n", rwin->free_space, rwin->last_in_seq);

    int i = 0;
    for(i = 0; i < MAX_WINDOW_SIZE; i++) {
        fprintf(stderr, "%d\t", i);
    }

    fprintf(stderr, "\n");

    for(i = 0; i < MAX_WINDOW_SIZE; i++) {
        if(rwin->buffer[i] != NULL) {
            fprintf(stderr, "#%d\t", pkt_get_seqnum(rwin->buffer[i]));
        } else {
            fprintf(stderr, "-\t");
        }
    }

    fprintf(stderr, "\n");

    for(i = 0; i < MAX_WINDOW_SIZE; i++) {
        if(rwin->buffer[i] != NULL) {
            fprintf(stderr, "L:%d\t", pkt_get_length(rwin->buffer[i]));
        } else {
            fprintf(stderr, "-\t");
        }
    }

    fprintf(stderr, "\n");
}

void free_rwindow(r_win * rwin)
{
	free(rwin);
	rwin = NULL;
}

bool shift_rwindow(r_win * rwin, int out_fd)
{
    int i;
    for(i = 0; i < MAX_WINDOW_SIZE; i++) {
        if(rwin->buffer[i] != NULL) {
            /* En principe toujours vrai, vu la façon dont on ajoute les élements
            dans la fenêtre */
            if(pkt_get_seqnum(rwin->buffer[i]) == ((rwin->last_in_seq + 1) % 256)) {
                /* Mise à jour du dernier numéro en séquence */
                rwin->last_in_seq = (rwin->last_in_seq + 1) % 256;

                /* Ecriture du pkt sur out_fd */
                ssize_t written_on_out = 0;
                while (written_on_out != pkt_get_length(rwin->buffer[i])) {
                    ssize_t ret = write(out_fd, (void *) pkt_get_payload(rwin->buffer[i]), pkt_get_length(rwin->buffer[i])-written_on_out);
                    if (ret == -1) {
                        perror("write()");
                    }

                    written_on_out += ret;
                }

                /* On libère l'espace alloué à ce paquet */
                pkt_del(rwin->buffer[i]);
                rwin->buffer[i] = NULL;

                rwin->free_space++;

                /* Si on arrive ici, c'est que la longueur du paquet reçu vaut 0,
                on est donc à la fin du transfert, on retourne true pour l'indiquer */
                if(written_on_out == 0) {
                    return true;
                }
            }
        } else {
            /* Si on arrive ici, c'est qu'il y un "trou" dans la fenête, il n'y
            a donc plus de paquets en séquence et on peut sortie de la boucle */
			break;
        }
    }

	/* Il faut ensuite décaler chaque élément de la fenêtre de i places vers la
    gauche, i étant le nombre de pkt en séquences trouvés dans la boucles
    précédentes. Si i == 0, il n'y a rien à faire. */
	int j;
	for(j = 0; j < MAX_WINDOW_SIZE - i && i != 0; j++) {
        if(rwin->buffer[j] != NULL && rwin->buffer[j+i] != NULL) {
            /* Nombre de bytes à copier = 4 (header) + longueur utile du
            payload + padding + 4 (crc) */
            size_t len = 8 + pkt_get_length(rwin->buffer[j+i]) + ((4 - (pkt_get_length(rwin->buffer[j+i]) % 4)) % 4);
            memcpy(rwin->buffer[j], rwin->buffer[j+i], len);
            rwin->buffer[j+i] = NULL;
        } else {
            rwin->buffer[j] = rwin->buffer[j+i];
            rwin->buffer[j+i] = NULL;
        }
    }

    /* Il ne reste plus qu'à vider les places à la fin de la fenêtre */
    int k;
    for(k = MAX_WINDOW_SIZE - 1; k > MAX_WINDOW_SIZE - 1 - i; k--) {
        pkt_del(rwin->buffer[k]);
        rwin->buffer[k] = NULL;
    }

    return false;
}

bool in_rwindow(r_win * rwin, uint8_t seqnum)
{
	int max = (rwin->last_in_seq + MAX_WINDOW_SIZE) % 256;
	int min = rwin->last_in_seq;

	/*
	 * La fenêtre ressemble à ça :
	 *      max          min
	 * ------]------------|-----
	 */
	if((rwin->last_in_seq + MAX_WINDOW_SIZE) >= 256) {
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

void build_ack(char ack[8], r_win * rwin) {
    /* Comme shift_rwindow s'éxécute après la fonction build_ack, il faut anticiper
    le nombre de places que va libérer cette fonction. On compte le nombre de pkt
    en séquence pour trouver le dernier en séquence et le nombre de place libre
    après l'exécution de shift_rwindow */
    int i = 0;
    while(i < MAX_WINDOW_SIZE && rwin->buffer[i] != NULL) {
        i++;
    }

    ack[0] = (uint8_t) (PTYPE_ACK << 5) | ((rwin->free_space + i) & 0b00011111);
    ack[1] = (rwin->last_in_seq + i + 1) % 256;
	ack[2] = 0;
	ack[3] = 0;
	uint32_t crc = (uint32_t) crc32(0, (Bytef *) ack, 4);
	ack[4] = (crc >> 24) & 0xFF;
	ack[5] = (crc >> 16) & 0xFF;
	ack[6] = (crc >> 8) & 0xFF;
	ack[7] = crc & 0xFF;
}

void build_nack(pkt_t * pkt, char nack[8], r_win * rwin) {
    /* On compte le nombre d'ack en séquence pour trouver le dernier en séquence
    et le nombre de place libre après l'exécution de shift_rwindow (pour la même
    raison que pour build_ack) */
    int i = 0;
    while(rwin->buffer[i] != NULL) {
        i++;
    }

    nack[0] = (uint8_t) (PTYPE_NACK << 5) | ((rwin->free_space + i) & 0b00011111);
    nack[1] = pkt_get_seqnum(pkt);
    nack[2] = 0;
	nack[3] = 0;
	uint32_t crc = (uint32_t) crc32(0, (Bytef *) nack, 4);
	nack[4] = (crc >> 24) & 0xFF;
	nack[5] = (crc >> 16) & 0xFF;
	nack[6] = (crc >> 8) & 0xFF;
	nack[7] = crc & 0xFF;
}

bool send_ack_or_nack(int sfd, char * buf, int size) {
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

void add_in_rwindow(pkt_t * d_pkt, r_win * rwin) {
    /* Si le paquet n'est pas encore dans la window */
	if(rwin->buffer[mod(pkt_get_seqnum(d_pkt) - (rwin->last_in_seq + 1), 256)] == NULL) {
		rwin->free_space--;   // Une place de moins dans la fenêtre
		rwin->buffer[mod(pkt_get_seqnum(d_pkt) - (rwin->last_in_seq + 1), 256)] = d_pkt;
	}
}

void receiver(int sfd, char * filename)
{
    /* Buffer pour stocker temporairement les données lues sur le socket */
    char buf[MAX_PKT_SIZE];

    /* file descriptor de la sortie du programme */
    int fd;

	/* Ouverture de filename s'il est spécifié */
	if(filename != NULL) {
		fd = open(filename, O_WRONLY|O_CREAT|O_TRUNC, 777);
		if(fd == -1) {
			perror("open()");
			return;
		}
	} else {
        fd = 1;     // sortie = stdout
    }

	/* Initialisation de la fenêtre de réception */
	r_win * rwin = init_rwindow();

    /* Variables utiles pour select */
    int retval;
    fd_set readfds, writefds;

	while(true)
	{
		FD_SET(sfd, &readfds);    // On peut lire sur le socket
        FD_SET(sfd, &writefds);   // On peut écrire sur le socket
		FD_SET(fd, &writefds);    // On peut écrire sur fd

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
                //fprintf(stderr, "There is something to read on the socket.\n");
				/* On lit les données sur le socket */
				ssize_t read_on_socket = read(sfd, (void *) buf, MAX_PKT_SIZE);
				if (read_on_socket == -1) {
                    perror("read()");
                    return;
                }

				/* On décode les données reçues dans d_pkt */
                pkt_t *d_pkt = pkt_new();
                pkt_status_code c = pkt_decode((const char *) buf, (const size_t) read_on_socket, d_pkt);

                /* Si le paquet reçu est valide et que le numéro de séquence
                   rentre dans le fenêtre de réception, on le traite */
                if(c == PKT_OK && in_rwindow(rwin, pkt_get_seqnum(d_pkt)))
                {
                    /* Si le packet lu est de type PTYPE_DATA... */
                    if(pkt_get_type(d_pkt) == PTYPE_DATA)
                    {
                        /* ... et qu'il contient plus de 4 bytes. Alors, on
                        le place dans la fenêtre de réception et on envoie
                        un ACK */
                        if(read_on_socket != 4) {
                            char ack[8];
							add_in_rwindow(d_pkt, rwin);
                            build_ack(ack, rwin);
                            if(!send_ack_or_nack(sfd, ack, 8))
                                return;
                        }
                        /* Sinon, le paquet a été coupé à cause de congestion
                        dans le réseau, il faut avertir le sender en envoyant
                        un NACK */
                        else {
                            char nack[8];
                            build_nack(d_pkt, nack, rwin);
                            if(!send_ack_or_nack(sfd, nack, 8))
                                return;
                        }
                    }
				} else {
				    /* Si le paquet est invalide, ou ne rentre pas dans la
                    fenêtre de réception : on ne fait rien, on l'ignore */
                }
            }

			/* On écrit sur out_fd (si des paquets en séquence ont été reçus) */
			if(shift_rwindow(rwin, fd)) {
                break;
            }
		}
	}

	/* Libération de l'espace alloué à la fenêtre de réception */
	free_rwindow(rwin);

	/* Femeture du fd de filename s'il était spécifié */
	if(filename != NULL) {
		int err = close(fd);
		if(err == -1)
			perror("close()");
	}
}
