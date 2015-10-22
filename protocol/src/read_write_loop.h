#ifndef __READ_WRITE_LOOP_H_
#define __READ_WRITE_LOOP_H_

#include <signal.h>
#include <time.h>

/* Structure of an element read by the sender on stdin or in a file
 * this element has 512 bytes + header (4) + CRC (4) (pkt_buf),
 * this element has a transmission timer (timerid)
 * this element has been acknowledged by the receiver (ack = 0) or not (ack = -1)
 */
struct window {
  char pkt_buf[520];
  int ack;
  timer_t * timerid;
};


/* Loop reading a socket and updating his attributes
 * while reading stdin or a file and writing to the socket
 * @sfd: The socket file descriptor. It is both bound and connected.
 * @filename: If a file is to be written on the socket instead of reading stdin
 * @return: as soon as the last ACK is received after stdin signals EOF
 */
void read_write_loop(const int sfd, char * filename);

#endif
