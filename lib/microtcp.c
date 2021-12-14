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
#include "../utils/log.h"

#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>
#include <sys/socket.h>
#include <sys/types.h>


/** TODO: move all helper functions to utils/helper.h and utils/helper.c */

#define MICROTCP_HEADER_SIZE sizeof(microtcp_header_t)
#define MIN2(x, y) ( (x > y) ? y : x )

void _cleanup(int status, void * recvbuf){

	free(recvbuf);

	if ( status == EXIT_SUCCESS )
		LOG_DEBUG("EXIT_SUCCESS\n");
	else
		LOG_DEBUG("EXIT_FAILURE\n");

	/** TODO: terminate connections ? */
}

/**
 * @brief Prepare the microTCP header for a packet to get send to the network
 * 
 * @param sock A valid microTCP socket handle
 * @param tcph microTCP header
 * @param ctrl Control bits
 * @param paysz Payload (data) size
 * @param payld Payload (data)
 */
void _preapre_send_tcph(microtcp_sock_t * sock, microtcp_header_t * tcph, uint16_t ctrlb, const void * payld, uint32_t paysz){

	if ( !sock ) { // debug only

		LOG_DEBUG("'sock' ---> NULL\n");
		check(-1);
	}

	if ( !tcph ) {  // debug only

		LOG_DEBUG("'tcph' ---> NULL\n");
		check(-1);
	}

	if ( (ctrlb == 3) ) {  // [SYN, FIN] together

		LOG_DEBUG("'ctrlb' ---> ");
		strctrl(ctrlb);
		check(-1);
	}

	tcph->seq_number = htonl(sock->seq_number);
	tcph->ack_number = htonl(sock->ack_number);
	tcph->control    = htons(ctrlb);
	tcph->window     = htons(sock->curr_win_size);
	tcph->data_len   = htonl(paysz);
	tcph->checksum   = htonl( (paysz) ? crc32(payld, paysz) : 0U );
}

void _handle_recv_tcph(microtcp_header_t * tcph){

	if ( !tcph ) {  // debug only, else error codes

		LOG_DEBUG("'tcph' ---> NULL\n");
		check(-1);
	}

	tcph->seq_number = ntohl(tcph->seq_number);
	tcph->ack_number = ntohl(tcph->ack_number);
	tcph->control    = ntohs(tcph->control);
	tcph->window     = ntohs(tcph->window);
	tcph->data_len   = ntohl(tcph->data_len);
	tcph->checksum   = ntohl(tcph->checksum);
}

int _timeout(int sockfd, int too){  // sockfd, timeout-option

	struct timeval to;  // timeout

	to.tv_sec = 0L;

	if ( too == TIOUT_ENABLE )
		to.tv_usec = MICROTCP_ACK_TIMEOUT_US;
	else  // timeout not-set
		to.tv_usec = 0L;
	
	check( setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &to, sizeof(to)) );

	return EXIT_SUCCESS;
}


//////////////////////////////////////////////////////////////////////////////////////


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
	
	// debug
	ackbase = sock.seq_number;
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
	void * tbuff;


	check(connect(socket->sd, address, address_len));

	if ( !( tbuff = malloc(MICROTCP_RECVBUF_LEN) ) ) {

		socket->state = INVALID;
		errno = ENOMEM;

		return -(EXIT_FAILURE);
	}

	check(on_exit(_cleanup, tbuff));

	tcph.seq_number = htonl(socket->seq_number);
	tcph.window     = htons(MICROTCP_RECVBUF_LEN);
	tcph.control    = htons(CTRL_SYN);
	socket->recvbuf = tbuff;

	check(send(socket->sd, &tcph, sizeof(tcph), 0));   // send SYN
	check(recv(socket->sd, &tcph, sizeof(tcph), 0));   // recv SYNACK

	// debug
	seqbase = ntohl(tcph.seq_number);  // necessary for print_tcp_header()
	print_tcp_header(socket, &tcph);

	/** SYNACK **/
	if ( ntohs(tcph.control) != (CTRL_SYN | CTRL_ACK) ) {

		socket->state = INVALID;
		errno = ECONNABORTED;

		return -(EXIT_FAILURE);
	}

	++socket->seq_number;
	socket->ack_number = ntohl(tcph.seq_number) + 1U;
	socket->sendbuflen = ntohs(tcph.window);

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
	void * tbuff;


	/** TODO: convert that to switch(){...}, add more states */

	if ( socket->state != INVALID )
		return -(EXIT_FAILURE);

	if ( !( tbuff = malloc(MICROTCP_RECVBUF_LEN) ) ) {

		errno = ENOMEM;
		return -(EXIT_FAILURE);
	}

	socket->recvbuf = tbuff;
	socket->state   = LISTEN;

	check(on_exit(_cleanup, tbuff));
	check(recvfrom(socket->sd, &tcph, sizeof(tcph), 0, address, &address_len));
	check(connect(socket->sd, address, address_len));

	// debug
	seqbase = ntohl(tcph.seq_number);  // necessary for print_tcp_header()
	print_tcp_header(socket, &tcph);

	if ( ntohs(tcph.control) != CTRL_SYN ) {

		socket->state = INVALID;
		errno = ECONNABORTED;

		return -(EXIT_FAILURE);
	}

	socket->sendbuflen = ntohs(tcph.window);
	socket->ack_number = ntohl(tcph.seq_number) + 1U;

	++socket->packets_received;
	++socket->bytes_received;

	tcph.seq_number = htonl(socket->seq_number);
	tcph.ack_number = htonl(socket->ack_number);
	tcph.control    = htons(CTRL_ACK | CTRL_SYN);
	tcph.window     = htons(MICROTCP_RECVBUF_LEN);

	check(send(socket->sd, &tcph, sizeof(tcph), 0));
	check(recv(socket->sd, &tcph, sizeof(tcph), 0));

	print_tcp_header(socket, &tcph);

	if ( ( ntohs(tcph.control) ) != CTRL_ACK ) {

		socket->state = INVALID;
		errno = ECONNABORTED;

		return -(EXIT_FAILURE);
	}

	socket->state = ESTABLISHED;

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
		check(send(socket->sd,(void*)&ack_header,sizeof(ack_header),0));
		
		if(socket->state==CLOSING_BY_PEER){
			microtcp_shutdown(socket, SHUT_WR);
			return EXIT_SUCCESS;
		}else if(socket->state==CLOSING_BY_HOST){
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
		check(send(socket->sd, (void*)&fin_ack, sizeof(fin_ack), 0));
		/* Receive ACK for previous FINACK */
		check(recv(socket->sd, (void*)&ack, sizeof(ack), 0));

		ack.control = ntohs(ack.control);

		/* Check if the received package is an ACK (and the correct ACK)*/
		if(ack.control & CTRL_ACK) {
			/** TODO: Check if ack is seq + 1 */
			if(socket->state == ESTABLISHED) {
				socket->state = CLOSING_BY_HOST;
			}
			else if(socket->state == CLOSING_BY_PEER) {
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

ssize_t microtcp_send(microtcp_sock_t * socket, const void * buffer, size_t length,
               int flags)
{
	uint8_t tbuff[MICROTCP_HEADER_SIZE + MICROTCP_MSS];  // c99 and onwards --- problem for larger MSS

	microtcp_header_t tcph;
	ssize_t ret;
	int sockfd;

	uint64_t bytes_to_send;
	uint64_t chunks;
	uint64_t index;
	uint64_t tmp;


	sockfd = socket->sd;

	while ( length ) {

		tmp = MIN2(socket->cwnd, socket->sendbuflen);
		bytes_to_send = MIN2(length, tmp);
		chunks = bytes_to_send / MICROTCP_MSS;

		LOG_DEBUG("\n\e[1mlength = %lu\e[0m\n"\
		          " > chunks = %lu\n"\
				  " > bytes_to_send = %lu\n", length, chunks, bytes_to_send);

		for ( index = 0UL; index < chunks; ++index ) {

			tmp = buffer + (index * MICROTCP_MSS);  // (void *) arithmetic ---> GNU C

			_preapre_send_tcph(socket, &tcph, 0U, tmp, MICROTCP_MSS);
			memcpy(tbuff, &tcph, MICROTCP_HEADER_SIZE);
			memcpy(tbuff + MICROTCP_HEADER_SIZE, tmp, MICROTCP_MSS);

			check(send(sockfd, tbuff, MICROTCP_MSS + MICROTCP_HEADER_SIZE, flags));
			socket->seq_number += MICROTCP_MSS;
		}

		// semi-filled chunk
		if ( bytes_to_send % MICROTCP_MSS ) {

			tmp = buffer + (index * MICROTCP_MSS);  // (void *) arithmetic ---> GNU C
			++chunks;

			_preapre_send_tcph(socket, &tcph, 0U, tmp, bytes_to_send);
			memcpy(tbuff, &tcph, MICROTCP_HEADER_SIZE);
			memcpy(tbuff + MICROTCP_HEADER_SIZE, tmp, bytes_to_send);

			check(send(sockfd, tbuff, bytes_to_send + MICROTCP_HEADER_SIZE, flags));
			socket->seq_number += bytes_to_send;
		}

		/** TODO: recv() every ACK/chunk */

		/** TODO: Retransmissions */
		/** TODO: Flow Control */
		/** TODO: Congestion control */

		length -= bytes_to_send;
	}


	return EXIT_SUCCESS;
}

ssize_t microtcp_recv(microtcp_sock_t * socket, void * buffer, size_t length, int flags)
{
	/** TODO: Flow Control */
	/** TODO: Packet reordering */
	/** TODO: Acknowledgements */
}
