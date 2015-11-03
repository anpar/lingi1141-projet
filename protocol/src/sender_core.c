#include "sender_core.h"

int lastack = 0;
int swin_free_space = SENDER_WINDOW_SIZE;
int rwin_free_space = MAX_WINDOW_SIZE;
int seqnum = 0;
int socket_number;
bool eof_reached = false;
/* Fast retransmission implementation */
bool forced = false;
int ack_received = 0;
int repetition = 0;

void init_swindow() {
    int j;
    for(j = 0; j < SENDER_WINDOW_SIZE; j++) {
        /* On crée les timers dans encore les armer */
        if(timer_create(CLOCK_REALTIME, NULL, &swin[j].timerid) != 0) {
            perror("timer_create()");
            return;
        }

        /* Initialement aucune "case" de la fenêtre n'attend d'ack */
        swin[j].ack = true;
    }
}

bool send_data(int sfd, char * buf, int size) {
    ssize_t written_on_socket = 0;
    while (written_on_socket != size) {
        ssize_t ret = write(sfd, (void *) buf, size-written_on_socket);
        if (ret == -1) {
            perror("write()");
            return(false);
        }

        written_on_socket += ret;
    }

    return(true);
}

void resend_data() {
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
    for(j = 0; j < SENDER_WINDOW_SIZE; j++) {
        /* si ack vaut false, alors on prend la valeur du timer, sinon la condition
        est forcément fausse et C n'évaluera même pas la deuxième, et ça tombe
        parce que si ack vaut true, le timer a été delete donc timer_gettime
        retourne une erreur */
        if(!swin[j].ack && timer_gettime(swin[j].timerid, &timerspec) != 0) {
            perror("timer_gettime()");
            return;
        }

        /* Si le timer est à 0 et que le paquet n'a pas encore été ack (ack == false)
        ou que forced vaut true */
        if((timerspec.it_value.tv_sec == 0 && timerspec.it_value.tv_nsec == 0) || forced) {
            /* On renvoie les données */
            if (!send_data(socket_number, swin[j].pkt_buf, swin[j].data_size)) {
                perror("write()");
                return;
            }

            /* On relance le timer */
            if(timer_settime(swin[j].timerid, 0, &itimerspec, NULL) != 0) {
                perror("timer_settime()");
                return;
            }
        }
    }
}

bool in_swindow(int seqnum_pkt) {
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
    for(i = 0; i < SENDER_WINDOW_SIZE; i++) {
        if(swin[i].ack != true) {
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
void shift_swindow(int k) {
    /* Pas d'élements acquis */
    if(k == 0) {
        return;
    }

    /* On garde en mémoire le nombre d'élement acquis */
    int num_ack = k;

    /* On décrémente k (il ne représente donc plus le nombre
    d'élement acquis mais l'indice du dernier élément dans la fenêtre
    d'envoi) */
    k--;

    /* Dans le cas ou la fenêtre est de la forme
    [.|...|255|0|...|.], on a k < 0 */
    if(k < 0) {
        k += 256;
        num_ack += 256;
    }

    /* On indique que des éléments ont été reçus */
    while (k >= 0) {
        swin[k].ack = true;

        /* On désarme le timer */
        if(timer_delete(swin[k].timerid) != 0) {
            perror("timer_delete();");
            return;
        }

        swin_free_space++;
        k--;
    }

    /* On décale tous les élements de la fenêtre */
    int j;
    for(j = 0; j < (SENDER_WINDOW_SIZE - num_ack); j++) {
        swin[j] = swin[j + num_ack];
        swin[j].timerid = swin[j + num_ack].timerid;
        swin[j].ack = swin[j + num_ack].ack;
        swin[j].data_size = swin[j + num_ack].data_size;
        memcpy(swin[j].pkt_buf, swin[j + num_ack].pkt_buf, swin[j + num_ack].data_size);
    }

    /* On crée num_ack nouvelles cases à la fin du tableau */
    while(j < SENDER_WINDOW_SIZE) {
        swin[j].ack = true;
        swin[j].data_size = 0;
        if(timer_create(CLOCK_REALTIME, NULL, &swin[j].timerid) != 0) {
            perror("timer_create()");
            return;
        }

        j++;
    }
}

void sender(int sfd, char * filename) {
    socket_number = sfd;

    /* Buffer pour lire les données sur stdin ou dans filename */
    char buf[MAX_PAYLOAD_SIZE];
    /* File descriptor de la sortie du programme */
    int fd;

    /* Lie le signal reçu à l'expriation d'un timer avec la fonction resend_data */
    signal(SIGALRM, resend_data);

    /* Initialisation de la fenêtre d'envoi */
    init_swindow();

    if(filename != NULL) {
        fd = open(filename, O_RDONLY);
        if(fd == -1) {
            perror("open()");
            return;
        }
    } else {
        fd = 0; // entrée = stdin
    }

    /* Variables utiles pour select */
    int retval;
    fd_set readfds, writefds;

    while (true) {
        FD_SET(fd, &readfds);     // On peut lire sur stdin/filename
        FD_SET(sfd, &writefds);   // On peut écrire sur le socket
        FD_SET(sfd, &readfds);    // On peut lire sur le socket

        retval = select(sfd+1, &readfds, &writefds, NULL, NULL);

        if (retval == -1) {
            perror("select()");
            return;
        } else if (!retval) {
            fprintf(stderr, "select() : timeout expires.\n");
            return;
        } else {
            /* Des données sont disponibles en lecture sur le socket */
            if (FD_ISSET(sfd, &readfds)) {
                ssize_t read_on_socket = read(sfd, buf, 8);
                if (read_on_socket == -1) {
                    perror("read()");
                    return;
                }

                /* On décode les données lues sur le socket, c'est forcément un
                ack ou un nack, donc la taille est forcément de 8 */
                pkt_t * pkt_temp = pkt_new();
                uint8_t seqnum_pkt;

                if(pkt_decode(buf, 8, pkt_temp) == PKT_OK) {
                    /* Si on reçoit un ack (attention : cumulatif), on essaye de
                    décaler la fenêtre de réception */
                    if(pkt_get_type(pkt_temp) == PTYPE_ACK) {
                        seqnum_pkt = pkt_get_seqnum(pkt_temp);

                        if(seqnum_pkt == ack_received) {
                            repetition++;
                            if(repetition == 3) {
                                forced = true;
                                resend_data();
                                forced = false;
                                repetition = 0;
                            }
                        } else {
                            ack_received = seqnum_pkt;
                            repetition = 0;
                        }

                        /* On met à jour le nombre de place disponibles dans la
                        fenêtre de réception de receiver */
                        rwin_free_space = pkt_get_window(pkt_temp);
                        if(in_swindow(seqnum_pkt)) {
                            // Nombre d'éléments cumulés reçus
                            int num_ack = seqnum_pkt - lastack;
                            shift_swindow(num_ack);
                            lastack = seqnum_pkt;

                            /* Si toute la fenêtre est ack, et qu'on a atteind EOF
                            sur filename/stdin, on a terminé le transfert */
                            if(is_window_acknowledged() && eof_reached)
                                return;
                        }
                    }
                    /* Si on reçoit un nack, on renvoie la donnée correspondante */
                    else if(pkt_get_type(pkt_temp) == PTYPE_NACK) {
                        seqnum_pkt = pkt_get_seqnum(pkt_temp);
                        /* On met à jour le nombre de place disponibles dans la
                        fenêtre de réception de receiver */
                        rwin_free_space = pkt_get_window(pkt_temp);
                        if(in_swindow(seqnum_pkt)) {
                            /* On trouve l'indice dans la fenêtre d'envoi correspondant
                            au paquet qui a été coupé */
                            int ind = seqnum_pkt - lastack;

                            /* Dans le cas ou la fenêtre ressemble à
                            [.|...|255|0|...|.], seqnum_pkt < 0 */
                            if(ind < 0) {
                                ind += 256;
                            }

                            swin[ind].ack = false;

                            /* Structures et variables pour relancer le timer */
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
                            if (!send_data(sfd, swin[ind].pkt_buf, swin[ind].data_size)) {
                                perror("write()");
                                return;
                            }

                            /* On relance le timer associé au paquet renvoyé */
                            if(timer_settime(swin[ind].timerid, 0, &itimerspec, NULL) != 0) {
                                perror("timer_settime()");
                                return;
                            }
                        }
                    } else {
                        /* Ni un ack ni un nack, on ne fait rien */
                    }
                } else {
                    /* Si le paquet reçu n'est pas valide, on ne fait rien */
                }

                pkt_del(pkt_temp);
            }

            /* Des données sont disponibles sur stdin ou sur le fichier spécifié
            par filename */
            if(!eof_reached &&  ((fd == 0   && FD_ISSET(fd, &readfds)   && FD_ISSET(sfd, &writefds) && swin_free_space > 0 && rwin_free_space > 0) ||
                                ( fd != 0                               && FD_ISSET(sfd, &writefds) && swin_free_space > 0 && rwin_free_space > 0))) {
                /* On lit les données sur l'entrée (stdin ou filename) */
                ssize_t read_on_in = read(fd, buf, MAX_PAYLOAD_SIZE);
                if (read_on_in == -1) {
                    perror("read()");
                    return;
                } else if (read_on_in == 0) {
                    eof_reached = true;
                }

                /* On trouve la première case de la fenêtre d'envoi qui a été
                acquise (et donc qui n'est plus nécessaire). */
                int i;
                for(i = 0; swin[i].ack != true; i++) {}

                /* Si on a trouvé une place dans la window */
                if(i != SENDER_WINDOW_SIZE) {
                    pkt_t * pkt_temp = pkt_new();
                    pkt_set_type(pkt_temp, PTYPE_DATA);
                    pkt_set_window(pkt_temp, swin_free_space-1);
                    pkt_set_seqnum(pkt_temp, seqnum);
                    pkt_set_length(pkt_temp, read_on_in);
                    pkt_set_payload(pkt_temp, buf, read_on_in);

                    size_t max = MAX_PKT_SIZE;
                    /* On encode en pkt pour obtenir le CRC et on place le résulat
                    dans la fenêtre d'envoi */
                    if(pkt_encode(pkt_temp, swin[i].pkt_buf, &max) == PKT_OK) {
                        swin[i].ack = false;
                        swin[i].data_size = max;
                        swin_free_space--; // Il y a une place de libre de moins

                        /* On écrit sur le socket */
                        if(!send_data(sfd, swin[i].pkt_buf, max)) {
                            perror("write()");
                            return;
                        }

                        /* Structures et variables pour lancer le timer */
                        struct timespec value;
                        struct timespec interval;
                        struct itimerspec itimerspec;

                        value.tv_sec = 5;
                        value.tv_nsec = 0;

                        interval.tv_sec = 0;
                        interval.tv_nsec = 0;

                        itimerspec.it_interval = interval;
                        itimerspec.it_value = value;

                        /* On lance le timer correspondant */
                        if(timer_settime(swin[i].timerid, 0, &itimerspec, NULL) != 0) {
                            perror("timer_settime()");
                            return;
                        }

                        seqnum++;
                        seqnum = seqnum % 256;
                    }

                    pkt_del(pkt_temp);
                }
            }
        }
    }
}
