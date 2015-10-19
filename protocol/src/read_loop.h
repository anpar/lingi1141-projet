#ifndef __READ_LOOP_H_
#define __READ_LOOP_H_

/* Loop reading a socket and writing on a socket.
 * This function decode the data read on the socket.
 * @sfd: The socket file descriptor. It is both bound and connected.
 * @filename: if != NULL, the file where data received must be stored.
 * @return: as soon as PTYPE_DATA with length = 0 is received
 */
void read_loop(const int sfd, char * filename);

#endif
