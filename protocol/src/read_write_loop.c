#include "read_write_loop.h"
#include "packet_interface.h"

#include "string.h" /* memset */

/*
    Permet d'empêcher le sender de continuer
    à essayer de lire sur stdin une fois que
    EOF a été atteint sur celui-ci.
*/
bool eof_reached = false;

/*
    Initialise la fenêtre en créant un timer non-armé par
    "case" de la fenêtre.
*/
void initWindow() {
    int j;
    for(j = 0; j < 32; j++) {
        if(timer_create(CLOCK_REALTIME, NULL, &windowTab[j].timerid) != 0) {
            perror("timer_create()");
            return;
        }

        windowTab[j].ack = 0;
    }
}

/*
void initTimer(struct timespec valuev, struct timespec intervali, struct itimerspec itimerspeci) {
    valuev.tv_sec = 5;
    valuev.tv_nsec = 0;

    intervali.tv_sec = 0;
    intervali.tv_nsec = 0;

    itimerspeci.it_interval = interval;
    itimerspeci.it_value = value;
}
*/

/*
* Permet d'écrire les données sur le socket.
* Renvoie true si tout s'est bien passé, false sinon.
*/
int write_on_socket2(int sfd, char * buf, int size) {
    ssize_t written_on_socket = 0;
    while (written_on_socket != size) {
        ssize_t ret = write(sfd, (void *) buf, size-written_on_socket);
        if (ret == -1) {
            perror("write()");
            return -1;
        }

        written_on_socket += ret;
    }

    return 0;
}

/*
    Permet de renvoyer un élément lorsque son timer est tombé à zéro
*/
void hdl() {
    struct itimerspec timerspec;

    struct timespec value;
    struct timespec interval;
    struct itimerspec itimerspec;

    value.tv_sec = 5;
    value.tv_nsec = 0;

    interval.tv_sec = 0;
    interval.tv_nsec = 0;

    itimerspec.it_interval = interval;
    itimerspec.it_value = value;

    int j;
    for(j = 0; j < 32; j++) {
        if(timer_gettime(windowTab[j].timerid, &timerspec) != 0) {
            perror("timer_gettime() %d");
            return;
        }

        if(timerspec.it_value.tv_sec == 0 && timerspec.it_value.tv_nsec == 0 && windowTab[j].ack == -1) {
            fprintf(stderr,"Le timer du paquet #%d a expiré ", (int) (lastack + j) % 256);

			pkt_t * p = pkt_new();
			pkt_decode(windowTab[j].pkt_buf, windowTab[j].data_size, p);
            /* On renvoie ce paquet */
            fprintf(stderr, "(seqnum : %d, type : %d).\n", pkt_get_seqnum(p), pkt_get_type(p));
            int ret = write_on_socket2(socketNumber, windowTab[j].pkt_buf, windowTab[j].data_size);
            if (ret == -1) {
                perror("write()");
                return;
            }

            /* On relance son timer */
            if(timer_settime(windowTab[j].timerid, 0, &itimerspec, NULL) != 0) {
                perror("timer_settime()");
                return;
            }
        }
    }
}

/*
    Check if the seqnum is in the range of the window
*/
bool seqnum_valid(int seqnum_pkt)
{
    if(seqnum_pkt < 0 || seqnum_pkt > 255) {
        return false;
    }

    int correction = 255 - lastack;
    if(correction < 32 && seqnum_pkt < (31-correction)) {
        return true;
    }

    if(seqnum_pkt >= lastack && seqnum_pkt <= lastack + 32) {
        return true;
    }

    return false;
}

/*
    Retourne vrai si la fenêtre est vide.
*/
bool is_window_acknowledged() {
    int i;
    for(i = 0; i < WINDOW_SIZE; i++) {
        if(windowTab[i].ack != 0) {
            return false;
        }
    }

    return true;
}

/*
    Met à jour le fenêtre d'envoi à la réception d'un ack.
    k représente le nombre d'éléments de la fenêtre de réception
    qui on été acquis.
*/
void slideWindowTab(int k) {
    fprintf(stderr, "Mise à jour de la fenêtre d'envoi.\n");

    /* Pas d'élements acquis */
    if(k == 0) {
        return;
    }

    int k2 = k;
    k = k-1;

    // case of [.|...|255|0|...|.]
    if(k < 0) {
        k = k + 256;
        k2 = k2 + 256;
    }

    // On vide les éléments qui ont été reçus
    fprintf(stderr, "Suppression des éléments bien reçus par le récepteur.\n");
    while (k >= 0) {
        windowTab[k].ack = 0;
        fprintf(stderr, "Place #%d libérée.\n", k);

        // FIX: Il faut quand même faire quelque chose avec le timer, le désarmé ou quoi
        if(timer_delete(windowTab[k].timerid) != 0) {
            perror("timer_delete();");
            return;
        }

        s_windowFree++;
        k--;
    }

    // On slide la fenêtre en deux boucles while
    fprintf(stderr, "Décalage de la fenêtre de réception.\n");
    int j;
    for(j = 0; j < (32-k2); j++){
        windowTab[j] = windowTab[k2+j];
        windowTab[j].timerid = windowTab[k2+j].timerid;
        windowTab[j].ack = windowTab[k2+j].ack;
        windowTab[j].data_size = windowTab[k2+j].data_size;
        memcpy(windowTab[j].pkt_buf, windowTab[k2+j].pkt_buf, windowTab[k2+j].data_size);
    }

    // On libère plusieurs cases au bout du tableau
    while(j < 32) {
        windowTab[j].ack = 0;
        windowTab[j].data_size = 0;
        if(timer_create(CLOCK_REALTIME, NULL, &windowTab[j].timerid) != 0) {
            perror("timer_create()");
            return;
        }

        j++;
    }

    return;
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
    socketNumber = sfd;
    int retval;
    fd_set readfds, writefds;
    char buf[BUF_SIZE]; // Permet de lire les données
    int fileDesc;       // Permet d'enregistrer si c'est stdin ou le fichier

    lastack = 0;
    /* FIX : 32 -> 31 */
    s_windowFree = 32;
    r_window_free = 31;
    seqnum_maker = 0;

    signal(SIGALRM, hdl); // Lie le signal à la fonction hdl()

    fprintf(stderr, "Initialisation de la fenêtre d'envoi.\n");
    initWindow();

    if(filename == NULL) {
        fprintf(stderr, "Pas de fichier spécifié, lecture sur stdin.\n");
        fileDesc = STDIN_FILENO;
    } else {
        fprintf(stderr, "Lecture depuis %s.\n", filename);
        fileDesc = open(filename, O_RDONLY);
        if(fileDesc == -1) {
            perror("open()");
            return;
        }
    }

    while (true) {
        FD_SET(fileDesc, &readfds);     // We can read on stdin/filename
        FD_SET(sfd, &writefds);         // We can write on the socket
        FD_SET(sfd, &readfds);          // We can read on the socket

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
                fprintf(stderr, "Reading on the socket : ");
                ssize_t read_on_socket = read(sfd, buf, 8);
                if (read_on_socket == -1) {
                    perror("read()");
                    return;
                } else if (read_on_socket == 0) {
                    fprintf(stderr, "(EOF).\n");
                }

                /* PRENDRE LES MESURES EN FONCTION DE CE QUI EST LU */
                pkt_t * pkt_temp = pkt_new();
                uint8_t seqnum_pkt;
                pkt_status_code p = pkt_decode(buf, 8, pkt_temp);
                if(p != PKT_OK) {
                    fprintf(stderr,"pkt_decode() a échoué et retourné %d.\n", (int) p);
                    return;
                }

                /* Si on reçoit un ack (attention:cumulatif), on modifie les données */
                if(pkt_get_type(pkt_temp) == PTYPE_ACK) {
					// TODO: implement fast retransmission
                    seqnum_pkt = pkt_get_seqnum(pkt_temp);
                    r_window_free = pkt_get_window(pkt_temp);
                    fprintf(stderr, "PTYPE_ACK reçu pour < %d (rwf = %d).\n", seqnum_pkt, r_window_free);
                    if(seqnum_valid(seqnum_pkt)) {
                        int k = seqnum_pkt - lastack; // Nombre d'éléments cumulés reçus
                        slideWindowTab(k);
                        lastack = seqnum_pkt;

                        if(is_window_acknowledged() && eof_reached)
                            return;
                    }
                } else if(pkt_get_type(pkt_temp) == PTYPE_NACK) {
                    int i = pkt_get_seqnum(pkt_temp);
                    r_window_free = pkt_get_window(pkt_temp);
                    fprintf(stderr, "PTYPE_NACK reçu pour < %d (rwf = %d).\n", i, r_window_free);
                    if(seqnum_valid(i)) {
                        i = i - lastack;
                        // case of [.|...|255|0|...|.]
                        if(i < 0) {
                            i = i+256;
                        }

                        windowTab[i].ack = -1;

                        struct timespec value;
                        struct timespec interval;
                        struct itimerspec itimerspec;

                        value.tv_sec = 5;
                        value.tv_nsec = 0;

                        interval.tv_sec = 0;
                        interval.tv_nsec = 0;

                        itimerspec.it_interval = interval;
                        itimerspec.it_value = value;

                        /* On renvoie le paquet */
                        int ret = write_on_socket2(sfd, windowTab[i].pkt_buf, windowTab[i].data_size);
                        if (ret == -1) {
                            perror("write()");
                            return;
                        }

                        /* On relance le timer associé au paquet renvoyé */
                        if(timer_settime(windowTab[i].timerid, 0, &itimerspec, NULL) != 0) {
                            perror("timer_settime()");
                            return;
                        }
                    }
                } else {
                    fprintf(stderr, "Ni un ACK ni a NACK.\n");
                    return;
                }

                pkt_del(pkt_temp);
            }

            /* Characacters become available for reading on stdin or in the file,
            we have to directly write those characacters on the socket. Need a free
            place in the windowtable */
            if(!eof_reached &&  ((fileDesc == STDIN_FILENO && FD_ISSET(fileDesc, &readfds) && FD_ISSET(sfd, &writefds) && s_windowFree > 0 && r_window_free > 0) ||
                                (fileDesc != STDIN_FILENO && FD_ISSET(sfd, &writefds) && s_windowFree > 0 && r_window_free > 0))) {
                fprintf(stderr, "Lecture de données et envoi vers le récepteur ");
                ssize_t read_on_stdin = read(fileDesc, buf, BUF_SIZE);
                if (read_on_stdin == -1) {
                    perror("read()");
                    return;
                } else if (read_on_stdin == 0) {
                    fprintf(stderr, "(EOF).\n");
                    eof_reached = true;
                }

                fprintf(stderr, "(%d bytes).\n", (int) read_on_stdin);

                /* FIX : correct de faire ça?
                Find the first free case of the window ack and timer initialised */
                int i = 0;
                while(windowTab[i].ack != 0) {
                    i++;
                }

                // Si on a trouvé une place dans la window
                if(i != WINDOW_SIZE) {
                    windowTab[i].ack = -1;
                    pkt_t * pkt_temp = pkt_new();
                    pkt_set_type(pkt_temp, PTYPE_DATA);
                    pkt_set_window(pkt_temp, s_windowFree-1);
                    pkt_set_seqnum(pkt_temp, seqnum_maker);
                    pkt_set_length(pkt_temp, read_on_stdin);
                    pkt_set_payload(pkt_temp, buf, read_on_stdin);
                    fprintf(stderr, "Nouveau paquet créé à partir des données lues, #%d.\n", (int) seqnum_maker);

                    size_t max = 520;
                    /* On encode en pkt pour obtenir le CRC et le mettre dans un buffer */
                    pkt_status_code p = pkt_encode(pkt_temp, windowTab[i].pkt_buf, &max);
                    windowTab[i].data_size = max;

                    if(p != PKT_OK) {
                        fprintf(stderr,"pkt_encode() a échoué et a retourné %d.\n", (int) p);
                        return;
                    }
                    s_windowFree--; // Il y a une place de libre de moins

                    /* On écrit sur le socket */
                    int ret = write_on_socket2(sfd, windowTab[i].pkt_buf, max);
                    if (ret == -1) {
                        perror("write()");
                        return;
                    }

                    struct timespec value;
                    struct timespec interval;
                    struct itimerspec itimerspec;

                    value.tv_sec = 5;
                    value.tv_nsec = 0;

                    interval.tv_sec = 0;
                    interval.tv_nsec = 0;

                    itimerspec.it_interval = interval;
                    itimerspec.it_value = value;

                    // On lance le timer correspondant
                    if(timer_settime(windowTab[i].timerid, 0, &itimerspec, NULL) != 0) {
                        perror("timer_settime()");
                        return;
                    }

                    pkt_del(pkt_temp);
                    seqnum_maker++;
                    seqnum_maker = seqnum_maker % 256;
                }
            }
        }
    }
}
