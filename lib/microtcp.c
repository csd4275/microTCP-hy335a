/*
 * microtcp, a lightweight implementation of TCP for teaching,
 * and academic purposes.
 *
 * Copyright (C) 2015-2017  Manolis Surligas <surligas@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "microtcp.h"
#include "../utils/crc32.h"
#include "../utils/errorc.h"

#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>
#include <sys/socket.h>
#include <sys/types.h>

microtcp_sock_t microtcp_socket(int domain, int type, int protocol)
{
	microtcp_sock_t sock;
	int sockfd;


	if ( type == SOCK_STREAM )
		type = SOCK_DGRAM;

	check(sockfd = socket(domain, type, protocol));
	memset(&sock, 0, sizeof(sock));
	srand(time(NULL) + getpid());

	sock.sd         = sockfd;
	sock.state      = INVALID;
	sock.seq_number = rand();
	sock.cwnd       = MICROTCP_INIT_CWND;
	sock.ssthresh   = MICROTCP_INIT_SSTHRESH;

	/** TODO: on_exit() -> free resources - call shutdown() */

	return sock;
}

int microtcp_bind(microtcp_sock_t * socket, const struct sockaddr * address,
               socklen_t address_len)
{
	check(bind(socket->sd, address, address_len));
	return EXIT_SUCCESS;
}

int microtcp_connect(microtcp_sock_t * socket, const struct sockaddr * address,
                  socklen_t address_len)
{
	microtcp_header_t tcph;


	tcph.seq_number = htonl(socket->seq_number);
	tcph.control    = htons(CTRL_SYN);

	check(connect(socket->sd, address, address_len));
	check(send(socket->sd, &tcph, sizeof(tcph), 0));   // send SYN
	check(recv(socket->sd, &tcph, sizeof(tcph), 0));   // recv SYNACK

	/** TODO: recvbuf handling */

	/** SYNACK **/
	if ( ntohs(tcph.control) != (CTRL_SYN | CTRL_ACK) ) {

		errno = ECONNABORTED;
		return -(EXIT_FAILURE);
	}

	++socket->seq_number;
	socket->ack_number = ntohl(tcph.seq_number) + 1U;

	tcph.seq_number = htonl(socket->seq_number);
	tcph.ack_number = htonl(socket->ack_number);
	tcph.control    = htons(CTRL_ACK);

	check(send(socket->sd, &tcph, sizeof(tcph), 0));  // send ACK
	socket->state = ESTABLISHED;

	return EXIT_SUCCESS;
}

int microtcp_accept(microtcp_sock_t * socket, struct sockaddr * address,
                 socklen_t address_len)
{
	microtcp_header_t tcph;


	/** TODO: convert that to switch(){...}, add more states */

	if ( socket->state != INVALID )
		return -(EXIT_FAILURE);

	socket->state = LISTEN;

	/** TODO: recvbuf setup */

	check(recvfrom(socket->sd, &tcph, sizeof(tcph), 0, address, &address_len));
	check(connect(socket->sd, address, address_len));

	if ( ntohs(tcph.control) != CTRL_SYN ) {

		errno = ECONNABORTED;
		return -(EXIT_FAILURE);
	}

	socket->state      = SYN_RCVD;
	socket->ack_number = ntohl(tcph.seq_number) + 1U;

	++socket->packets_received;
	++socket->bytes_received;

	/** TODO: cwnd and receive buffer */

	tcph.seq_number = htonl(socket->seq_number);
	tcph.ack_number = htonl(socket->ack_number);
	tcph.control    = htons(CTRL_ACK | CTRL_SYN);

	check(send(socket->sd, &tcph, sizeof(tcph), 0));
	check(recv(socket->sd, &tcph, sizeof(tcph), 0));

	if ( ( ntohs(tcph.control) ) != CTRL_ACK ) {

		errno = ECONNABORTED;
		return -(EXIT_FAILURE);
	}

	socket->state = ESTABLISHED;

	/** TODO: implement checksum() in every recvfrom() */
	/** TODO: implement checksum() */

	return EXIT_SUCCESS;
}

int microtcp_shutdown(microtcp_sock_t * socket, int how)
{	
	switch (how)
	{
	case SHUT_RD:

		if(socket->state==ESTABLISHED){
			socket->state=CLOSING_BY_PEER;
		}else if(socket->state==CLOSING_BY_HOST){
			// socket->state= ?? state becomes closed  on SHUT_RDWR
		}
		else
			return -(EXIT_FAILURE);

		microtcp_header_t ack_header;
		ack_header.control = htons(CTRL_ACK);
		ack_header.ack_number = htonl(socket->ack_number);
		printf("Sending ACK\n");
		check(send(socket->sd,(void*)&ack_header,sizeof(ack_header),0));
		
		if(socket->state==CLOSING_BY_PEER){
			printf("state is CBP mtcp_shut called (how == 1)\n");
			microtcp_shutdown(socket, SHUT_WR);
			return EXIT_SUCCESS;
		}else if(socket->state==CLOSING_BY_HOST){
			printf("state is CBH mtcp_shut called (how == 2)\n");
			microtcp_shutdown(socket,SHUT_RDWR);
		}else{
			return -(EXIT_FAILURE);
		}

		break;
	case SHUT_WR:
		printf("SHUT_WR\n");
		microtcp_header_t fin_ack, ack;

		fin_ack.seq_number = htonl(socket->seq_number);
		fin_ack.ack_number = htonl(socket->ack_number);
		fin_ack.control = htons(CTRL_FIN | CTRL_ACK);
		/* Send FIN/ACK */
		printf("Sending ACK\n");
		check(send(socket->sd, (void*)&fin_ack, sizeof(fin_ack), 0));
		/* Receive ACK for previous FINACK */
		printf("Waiting for response..\n");
		check(recv(socket->sd, (void*)&ack, sizeof(ack), 0));

		ack.control = ntohs(ack.control);

		/* Check if the received package is an ACK (and the correct ACK)*/
		if(ack.control & CTRL_ACK) {
			printf("Response is indeed ACK\n");
			/** TODO: Check if ack is seq + 1 */
			if(socket->state == ESTABLISHED) {
				printf("State is now CBH\n");
				socket->state = CLOSING_BY_HOST;
			}
			else if(socket->state == CLOSING_BY_PEER) {
				printf("State is CBP so calling SHUT_RDWR\n");
				microtcp_shutdown(socket, SHUT_RDWR);
			}
		}

		break;
	case SHUT_RDWR: /* SHUT _RDWR */
		printf("SHUT_RDWR\n");
		printf("Closing socket\n");
		close(socket->sd);
		socket->state = CLOSED;
		break;
	default:
		return EINVAL;
	}
}

ssize_t microtcp_send(microtcp_sock_t * socket, const void *buffer, size_t length,
               int flags)
{
	/* Your code here */
}

ssize_t microtcp_recv(microtcp_sock_t * socket, void * buffer, size_t length, int flags)
{
	/* Your code here */
}
