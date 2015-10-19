#ifndef __WAIT_FOR_SENDER_H_
#define __WAIT_FOR_SENDER_H_

/* Block the receiver until a packet is received on sfd,
 * and connect the socket to the sender address of the received packet.
 * @sfd: a file descriptor to a bound socket but not yet connected
 * @return: 0 in case of success, -1 otherwise
 * @POST: This call is idempotent, it does not 'consume' the data of the packet,
 * and could be repeated several times blocking only at the first call.
 */
int wait_for_sender(int sfd);

#endif
