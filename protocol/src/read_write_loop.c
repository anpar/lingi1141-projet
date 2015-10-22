#include "read_write_loop.h"
#include "packet_interface.h"




// Permet de mettre toutes les places disponibles dans le buffer
void initWindow()
{
    int j = 0;
    while(j < 32)
    {
        windowTab[j].ack = 0;
        j++;
    }
}

/*
void initTimer(struct timespec valuev, struct timespec intervali, struct itimerspec itimerspeci)
{
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
        ssize_t ret = write (sfd, (void *) buf, size-written_on_socket);
        if (ret == -1) {
            perror("write()");
            return -1;
        }

        written_on_socket += ret;
    }

    return 0;
}

//Permet de renvoyer un élément lorsque son timer est tombé à zéro
void hdl()
{
    struct itimerspec timerspec;

    struct timespec value;
    struct timespec interval;
    struct itimerspec itimerspec;
    //    initTimer(value, interval, itimerspec);
    value.tv_sec = 5;
    value.tv_nsec = 0;

    interval.tv_sec = 0;
    interval.tv_nsec = 0;

    itimerspec.it_interval = interval;
    itimerspec.it_value = value;



    int j = 0;
    while(j < 32)
    {
        if(timer_gettime(windowTab[j].timerid, &timerspec)!=0)
        {
            perror("timer_gettime() %d");
            return;
        }
        if(timerspec.it_value.tv_sec==0)
        {
            /* On écrit sur le socket */
            int ret = write_on_socket2(socketNumber, windowTab[j].pkt_buf, 520);
            if (ret == -1) {
                perror("write()");
                return;
            }
            if(timer_settime(windowTab[j].timerid,0,&itimerspec,NULL)!=0) // RELANCER TIMER
            {
                perror("timer_settime()");
                return;
            }
            return;
        }
        j++;
    }
}

/*
* Check if the seqnum is in the range of the window
*/
int seqnum_valid(int seqnum_pkt)
{
    if(seqnum_pkt < 0 || seqnum_pkt >255)
    {
        return -1;
    }
    int correction = 255 - lastack;
    if(correction < 32 && seqnum_pkt < (32-correction))
    {
        return 0;
    }
    if(seqnum_pkt >= lastack && seqnum_pkt < lastack + 32)
    {
        return 0;
    }
    return -1;
}


/*
* Update windowTab when receiving an ACK
*/
void slideWindowTab(int k)
{
    int k2 = k+1;
    if(k<0) // case of [.|...|255|0|...|.]
    {
        k = k + 256;
    }
    while (k >= 0) // On vide les éléments qui ont été reçus
    {
        windowTab[k].ack = 0;
        timer_delete(*windowTab[k].timerid);
        windowFree ++;
        k--;
    }
    int j = 0;
    while(j < (32-k2)) // On slide la fenêtre en deux boucles while
    {
        windowTab[j] = windowTab[k2+j];
        j++;
    }
    while(j <= 32) // On libère plusieurs cases au bout du tableau
    {
        windowTab[j].ack = 0;
        timer_delete(*windowTab[j].timerid);
    }
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
    int fileDesc; // Permet d'enregistrer si c'est stdin ou le fichier

    lastack = 0;
    windowFree = 32;
    seqnum_maker = 0;


    struct timespec value;
    struct timespec interval;
    struct itimerspec itimerspec;

    value.tv_sec = 5;
    value.tv_nsec = 0;

    interval.tv_sec = 0;
    interval.tv_nsec = 0;

    itimerspec.it_interval = interval;
    itimerspec.it_value = value;



    signal(SIGALRM, hdl); // Lie le signal à la fonction hdl()

    initWindow();
    //    initTimer(value, interval, itimerspec);



    while (1) {
        if(filename == NULL)
        {
            fileDesc = STDIN_FILENO;
        }
        else
        {
            fileDesc = open(filename,O_RDONLY);
        }
        FD_SET(fileDesc, &readfds);     // We can read on stdin
        FD_SET(sfd, &writefds);             // We can write on the socket
        FD_SET(sfd, &readfds);              // We can read on the socket

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
                fprintf(stderr, "Reading on the socket and writing on stdout ");
                ssize_t read_on_socket = read(sfd, buf, 8);
                if (read_on_socket == -1) {
                    perror("read()");
                    return;
                } else if (read_on_socket == 0) {
                    fprintf(stderr, "(EOF).\n");
                    return;
                }

                fprintf(stderr, "(%d bytes).\n", (int) read_on_socket);

                /* PRENDRE LES MESURES EN FONCTION DE CE QUI EST LU */
                pkt_t * pkt_temp = pkt_new();
                uint8_t seqnum_pkt;
                if(pkt_decode(buf, 8, pkt_temp) != PKT_OK)
                {
                    perror("pkt_decode()");
                    return;
                }
                /* Si on reçoit un ack (attention:cumulatif), on modifie les données */
                if(pkt_get_type(pkt_temp) == PTYPE_ACK)
                {
                    seqnum_pkt = pkt_get_seqnum(pkt_temp);
                    if(seqnum_valid(seqnum_pkt)==0)
                    {
                        int k = seqnum_pkt - lastack; // Nombre d'éléments cumulés reçus
                        slideWindowTab(k);
                        lastack = seqnum_pkt;
                    }
                }
                else if(pkt_get_type(pkt_temp) == PTYPE_NACK)
                {
                    int i = pkt_get_seqnum(pkt_temp);
                    if(seqnum_valid(i)==0)
                    {
                        i = i - lastack;
                        if(i < 0) // case of [.|...|255|0|...|.]
                        {
                            i = i+256;
                        }
                        windowTab[i].ack = -1;

                        /* On écrit sur le socket */
                        int ret = write_on_socket2(sfd, windowTab[i].pkt_buf, 520);
                        if (ret == -1) {
                            perror("write()");
                            return;
                        }
                        if(timer_settime(windowTab[i].timerid,0,&itimerspec,NULL)!=0) // RELANCER TIMER
                        {
                            perror("timer_settime()");
                            return;
                        }
                    }
                }
                else
                {
                    perror("Nor an ACK nor a NACK");
                    return;
                }
                pkt_del(pkt_temp);

            }


            /* Characacters become available for reading on stdin or in the file,
            we have to directly write those characacters on the socket. Need a free
            place in the windowtable */
            if (FD_ISSET(fileDesc, &readfds) && FD_ISSET(sfd, &writefds) && windowFree > 0) {
                fprintf(stderr, "Reading on fileDesc and writing on the socket ");
                ssize_t read_on_stdin = read(fileDesc, buf, BUF_SIZE);
                if (read_on_stdin == -1) {
                    perror("read()");
                    return;
                } else if (read_on_stdin == 0) {
                    fprintf(stderr, "(EOF).\n");
                    return;
                }

                fprintf(stderr, "(%d bytes).\n", (int) read_on_stdin);

                /* Find the first free case of the window
                ack and timer initialised*/
                int i;
                while(windowTab[i].ack != 0)
                {
                    i++;
                }
                windowTab[i].ack = -1;
                if(timer_create(CLOCK_REALTIME,NULL,windowTab[i].timerid)!=0)
                {
                    perror("timer_create()");
                    return;
                }

                pkt_t * pkt_temp = pkt_new();
                pkt_set_type(pkt_temp, PTYPE_DATA);
                pkt_set_window(pkt_temp, windowFree);
                pkt_set_seqnum(pkt_temp, seqnum_maker);
                pkt_set_length(pkt_temp, read_on_stdin);

                /* On encode en pkt pour obtenir le CRC et le mettre dans un buffer */
                if(pkt_encode(pkt_temp, windowTab[i].pkt_buf, (size_t *) &read_on_stdin) != PKT_OK)
                {
                    perror("pkt_encode()");
                    return;
                }
                windowFree --; // Il y a une place de libre de moins

                /* On écrit sur le socket */
                int ret = write_on_socket2(sfd, windowTab[i].pkt_buf, (read_on_stdin + 8));
                if (ret == -1) {
                    perror("write()");
                    return;
                }
                if(timer_settime(windowTab[i].timerid,0,&itimerspec,NULL)!=0) //LANCER TIMER
                {
                    perror("timer_settime()");
                    return;
                }
                pkt_del(pkt_temp);
                seqnum_maker ++;
                seqnum_maker = seqnum_maker % 256;
            }
        }
    }
}
