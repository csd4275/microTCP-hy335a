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
#include <time.h>

/** DEFINES **/
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
	check(bind(socket->sd, address, address_len));
	return EXIT_SUCCESS;
}

int microtcp_connect(microtcp_sock_t * socket, const struct sockaddr * address,
                  socklen_t address_len)
{
	/* Your code here */
}

int microtcp_accept(microtcp_sock_t * socket, struct sockaddr * address,
                 socklen_t address_len)
{
	char buff[MICROTCP_MSS];
	int sockfd;

	microtcp_header_t tcphr;
	microtcp_header_t tcphs;


	/** TODO: 3-way-handshake */
	/** TODO: recvbuf setup */
	sockfd = socket->sd;
	check(recvfrom(sockfd, buff, MICROTCP_MSS + 40U, 0, address, &address_len));
	memcpy(&tcphr, buff, sizeof(tcphr));

	tcphr.seq_number = ntohl(tcphr.seq_number);
	tcphr.ack_number = ntohl(tcphr.ack_number);
	tcphr.control    = ntohs(tcphr.control);
	tcphr.window     = ntohs(tcphr.window);
	tcphr.data_len   = ntohs(tcphr.data_len);

	printf("header.control = %x\n", tcphr.control);

	if ( tcphr.control & CTRL_SYN ) {

		printf("CTRL-SYN\n");
		socket->ack_number = tcphr.seq_number + 1U;
		++socket->packets_received;
		++socket->bytes_received;

		/** TODO: cwnd and receive buffer */

		tcphs.seq_number = socket->seq_number;
		tcphs.ack_number = socket->ack_number;
		tcphs.control    = CTRL_ACK | CTRL_SYN;
	}

	/** TODO: implement checksum() */

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
