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
#include <arpa/inet.h>


/** DEFINES **/
#define SIZE_TCPH sizeof(microtcp_header_t)

#define CTRL_FIN ( 1 << 0 )
#define CTRL_SYN ( 1 << 1 )
#define CTRL_RST ( 1 << 2 )
#define CTRL_ACK ( 1 << 4 )

microtcp_sock_t microtcp_socket(int domain, int type, int protocol)
{
	microtcp_sock_t sock;
	int sockfd;


	check(sockfd = socket(domain, type, protocol));
	memset(&sock, 0, sizeof(sock));
	srand(time(NULL) + getpid());

	sock.sd         = sockfd;
	sock.state      = INVALID;
	sock.seq_number = rand();

	return sock;
}

int microtcp_bind(microtcp_sock_t * socket, const struct sockaddr * address,
               socklen_t address_len)
{
	memccpy((void*)&socket->addr,(void*)address,'\0',sizeof(address));

	check(bind(socket->sd, address, address_len));
	return EXIT_SUCCESS;
}

int microtcp_connect(microtcp_sock_t * socket, const struct sockaddr * address,
                  socklen_t address_len)
{
	microtcp_header_t estab_header;
	estab_header.seq_number=socket->seq_number;
	estab_header.control= SYN;

	check(sendto(socket->sd,(void*)&estab_header,sizeof(estab_header),NULL,(struct sockaddr *)&socket->addr,sizeof(socket->addr)));
	socket->seq_number+=sizeof(estab_header);


	// recv(socket->sd,)



	microtcp_header_t ack_estab_header;
	estab_header.seq_number=socket->seq_number;
	estab_header.control= ACK;

	check(sendto(socket->sd,(void*)&estab_header,sizeof(ack_estab_header),NULL,(struct sockaddr *)&socket->addr,sizeof(socket->addr)));
	socket->seq_number+=sizeof(ack_estab_header);

	socket->state=ESTABLISHED;
	return socket->sd;
}

int microtcp_accept(microtcp_sock_t * socket, struct sockaddr * address,
                 socklen_t address_len)
{
	microtcp_header_t tcph;


	if ( !socket || !address ) {

		errno = EINVAL;
		return -(EXIT_FAILURE);
	}

	if ( socket->state == ESTABLISHED ) {

		errno = EISCONN;
		return -(EXIT_FAILURE);
	}

	/** TODO: 3-way-handshake */
	/** TODO: recvbuf setup */

	check(recvfrom(socket->sd, &tcph, SIZE_TCPH, 0, address, &address_len));

	tcph.seq_number = ntohl(tcph.seq_number);
	tcph.ack_number = ntohl(tcph.ack_number);
	tcph.control    = ntohs(tcph.control);
	tcph.window     = ntohs(tcph.window);
	tcph.data_len   = ntohs(tcph.data_len);

	printf("header.control = %x\n", tcph.control);

	if ( !(tcph.control & CTRL_SYN) ) {

		errno = ECONNABORTED;
		return -(EXIT_FAILURE);
	}

	printf("CTRL-SYN\n");
	socket->ack_number += tcph.data_len;
	++socket->packets_received;
	++socket->bytes_received;

	/** TODO: cwnd and receive buffer */

	tcph.seq_number = htonl(socket->seq_number);
	tcph.ack_number = htonl(socket->ack_number);
	tcph.control    = htons(CTRL_ACK | CTRL_SYN);
	tcph.data_len   = htonl(1U);

	check(sendto(socket->sd, &tcph, SIZE_TCPH, 0, address, &address_len));
	check(recvfrom(socket->sd, &tcph, SIZE_TCPH, 0, address, &address_len));

	tcph.seq_number = ntohl(tcph.seq_number);
	tcph.ack_number = ntohl(tcph.ack_number);
	tcph.control    = ntohs(tcph.control);
	tcph.window     = ntohs(tcph.window);
	tcph.data_len   = ntohs(tcph.data_len);

	if ( !(tcph.control & CTRL_ACK) ) {

		/** TODO: while ( !ack_not_recvd ); */

		printf("no ACK recvd!\n");
		return -(EXIT_FAILURE);
	}

	printf("ACKed\n");

	/** TODO: implement checksum() in every recvfrom() */

	return EXIT_SUCCESS;
}

int microtcp_shutdown(microtcp_sock_t * socket, int how)
{
	/* Your code here */
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
