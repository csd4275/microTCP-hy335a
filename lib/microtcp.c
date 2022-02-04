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


#define MICROTCP_HEADER_SIZE sizeof(microtcp_header_t)
#define MIN2(x, y) ( (x > y) ? y : x )
#define _ntoh_recvd_tcph(microtcp_header)  \
								{\
									microtcp_header.seq_number = ntohl(tcph.seq_number);\
									microtcp_header.ack_number = ntohl(tcph.ack_number);\
									microtcp_header.control    = ntohs(tcph.control);\
									microtcp_header.window     = ntohs(tcph.window);\
									microtcp_header.data_len   = ntohl(tcph.data_len);\
									microtcp_header.checksum   = ntohl(tcph.checksum);\
								}

#define TIOUT_DISABLE  0
#define TIOUT_ENABLE   1


/**
 * @brief Initializes the microTCP header for a packet to get send over the network. By giving FRAGMENT
 * in 'ctrlb', the packet (header) will be marked as fragmented. Putting CTRL_XXX in 'ctrlb' will not
 * set any control bits in the header
 * 
 * @param sock a valid microTCP socket handle
 * @param tcph microTCP header
 * @param ctrl control bits
 * @param paysz payload size
 * @param payld payload
 */
static void _preapre_send_tcph(microtcp_sock_t * __restrict__ sock, microtcp_header_t * __restrict__ tcph, uint16_t ctrlb,
						const void * __restrict__ payld, uint32_t paysz){

	#ifdef ENABLE_DEBUG_MSG
	if ( !sock ) {

		LOG_DEBUG("'sock' ---> NULL\n");
		check(-1);
	}

	if ( !tcph ) {

		LOG_DEBUG("'tcph' ---> NULL\n");
		check(-1);
	}
	#endif

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

/**
 * @brief ENABLE or DISABLE the timeout socket option
 * @param sockfd A valid socket
 * @param too timeout-option (TIOUT_ENABLE, TIOUT_DISABLE)
 * @return int 
 */
static int _timeout(int sockfd, int too){

	struct timeval to;  // timeout

	to.tv_sec = 0L;

	if ( too == TIOUT_ENABLE )
		to.tv_usec = MICROTCP_ACK_TIMEOUT_US;
	else  // timeout not-set
		to.tv_usec = 0L;
	
	check( setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &to, sizeof(to)) );

	return EXIT_SUCCESS;
}

static void cleanup();

//////////////////////////////////////////////////////////////////////////////////////
/** TODO: [!] implement byte and packet statistics [!] */

microtcp_sock_t microtcp_socket(int domain, int type, int protocol)
{
	microtcp_sock_t sock;

	int sockfd;
	int optval;
	int optsz;


	if ( type != SOCK_DGRAM )
		LOG_DEBUG("type of socket changed to 'SOCK_DGRAM'\n");

	bzero(&sock, sizeof(sock));
	sock.recvbuf = (uint8_t *) malloc(MICROTCP_RECVBUF_LEN);

	if ( !sock.recvbuf ) {

		sock.sd    = -1;
		sock.state = INVALID;
		sock.state = CLOSED;
		errno = ENOMEM;

		return sock;
	}

	optsz  = sizeof(int);
	optval = MICROTCP_RECVBUF_LEN;

	check( sockfd = socket(domain, SOCK_DGRAM, protocol ));
	check( setsockopt(sockfd, SOL_SOCKET, SO_RCVBUF, &optval, optsz) );  // The kernel will double this amount
	memset(&sock, 0, sizeof(sock));
	srand(time(NULL) + getpid());

	sock.sd         = sockfd;
	sock.seq_number = rand();
	sock.cwnd       = MICROTCP_INIT_CWND;
	sock.ssthresh   = MICROTCP_INIT_SSTHRESH;
	
	#ifdef ENABLE_DEBUG_MSG
	ackbase = sock.seq_number;
	#endif
	
	

	return sock;
}

int microtcp_bind(microtcp_sock_t * __restrict__ socket, const struct sockaddr * __restrict__ address,
               socklen_t address_len)
{
	check(bind(socket->sd, address, address_len));
	return EXIT_SUCCESS;
}

int microtcp_connect(microtcp_sock_t * __restrict__ socket, const struct sockaddr * __restrict__ address,
                  socklen_t address_len)
{
	microtcp_header_t tcph;
	int64_t sockfd;


	if ( !socket ) {

		errno = EINVAL;
		return -(EXIT_FAILURE);
	}

	check(connect(socket->sd, address, address_len));

	tcph.seq_number = htonl(socket->seq_number);
	tcph.window     = htons(MICROTCP_RECVBUF_LEN);
	tcph.control    = htons(CTRL_SYN);

	check(send(socket->sd, &tcph, sizeof(tcph), 0));   // send SYN
	check(recv(socket->sd, &tcph, sizeof(tcph), 0));   // recv SYNACK

	#ifdef ENABLE_DEBUG_MSG
	seqbase = ntohl(tcph.seq_number);  // necessary for print_tcp_header()
	#endif
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

int microtcp_accept(microtcp_sock_t * __restrict__ socket, struct sockaddr * __restrict__ address,
                 socklen_t address_len)
{
	microtcp_header_t tcph;


	/** TODO: convert that to switch(){...}, add more states */

	if ( socket->state != INVALID )
		return -(EXIT_FAILURE);

	socket->state   = LISTEN;

	check( recvfrom(socket->sd, &tcph, sizeof(tcph), 0, address, &address_len) );
	check( connect(socket->sd, address, address_len) );

	#ifdef ENABLE_DEBUG_MSG
	seqbase = ntohl(tcph.seq_number);  // necessary for print_tcp_header()
	#endif
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

	++socket->seq_number;
	socket->state = ESTABLISHED;

	/** TODO: implement checksum() */

	return EXIT_SUCCESS;
}

int microtcp_shutdown(microtcp_sock_t * socket, int how)
{
	// LOG_DEBUG("Start of SD");
	// LOG_DEBUG("how=%d",how);
	microtcp_header_t fin_ack, ack;

	if(how==SHUTDOWN_CLIENT){//sender is shutting down the connection

		fin_ack.seq_number = htonl(socket->seq_number);
		fin_ack.ack_number = htonl(socket->ack_number);
		fin_ack.control    = htons(CTRL_FIN | CTRL_ACK);

		/* Send FIN/ACK */
		check(send(socket->sd, (void*)&fin_ack, sizeof(fin_ack), 0));
		/* Receive ACK for previous FINACK */
		check(recv(socket->sd, (void*)&ack, sizeof(ack), 0));

		uint16_t recieved_ack = ntohs(ack.control);

		/* Check if the received package is an ACK (and the correct ACK)*/
		if(!(recieved_ack & CTRL_ACK)){
			return -(EXIT_FAILURE);
		}

		socket->state = CLOSING_BY_HOST;

		/* Wait for FIN ACK from the server*/
		check(recv(socket->sd, (void*)&fin_ack, sizeof(ack), 0))

		uint16_t recieved_finack = ntohs(fin_ack.control);

		if(!(recieved_finack & (CTRL_FIN | CTRL_ACK))){
			return -(EXIT_FAILURE);
		}

		/* Prepare and send back the ACK header*/
		ack.control    = htons(CTRL_ACK);
		ack.ack_number = htonl(socket->ack_number);
		ack.seq_number = htonl(socket->seq_number);

		check(send(socket->sd, (void*)&ack, sizeof(ack), 0));
		
		/** TODO: Timed wait for server FIN ACK retransmition */
		socket->state = CLOSED;
		return EXIT_SUCCESS;

	}else if(how==SHUTDOWN_SERVER){//reciever recieved a FIN packet

		socket->state=CLOSING_BY_PEER;
		// LOG_DEBUG("SD: state:cbp");
		ack.control    = htons(CTRL_ACK);
		ack.ack_number = htonl(socket->ack_number);
		check(send(socket->sd, (void*)&ack, sizeof(ack), 0));
		
		// LOG_DEBUG("SD: sent ACK\n");
		fin_ack.seq_number = htonl(socket->seq_number);
		fin_ack.ack_number = htonl(socket->ack_number);
		fin_ack.control    = htons(CTRL_FIN | CTRL_ACK);
		
		/* Send FIN/ACK */
		check(send(socket->sd, (void*)&fin_ack, sizeof(fin_ack), 0));
		// LOG_DEBUG("SD: Sent FINACK\n");
		// LOG_DEBUG("SD: Waiting for ACK\n");
		/* Receive ACK for previous FINACK */
		check(recv(socket->sd, (void*)&ack, sizeof(ack), 0));

		uint16_t recieved_ack = ntohs(ack.control);

		/* Check if the received package is an ACK */
		if((recieved_ack & CTRL_ACK)) {
			return -(EXIT_FAILURE);
		}
		// LOG_DEBUG("SD: recieved ACK");

		/* Terminate the connection */
		socket->state = CLOSED;
		// LOG_DEBUG("SD: state:closed");
		return EXIT_SUCCESS;

	}else{
		errno = EINVAL;
		return -(EXIT_FAILURE);
	}
}

ssize_t microtcp_send(microtcp_sock_t * __restrict__ socket, const void * __restrict__ buffer, size_t length,
               int flags)
{
	/** TODO: Retransmissions (timeout & 3dup ACKs) */
	/** TODO: Flow Control */
	/** TODO: Congestion control */

	uint8_t tbuff[MICROTCP_HEADER_SIZE + MICROTCP_MSS];  // c99 and onwards --- problem for larger MSS

	microtcp_header_t tcph;

	int sockfd;
	int fflag;  // fragments flag

	size_t lengthcpy = length;

	register uint64_t bytes_to_send;
	register uint64_t chunks;
	register uint64_t index;
	register uint64_t tmp;


	if ( !socket ) {

		errno = EINVAL;
		return -(EXIT_FAILURE);
	}

	if ( (socket->state == INVALID) || (socket->state >= CLOSING_BY_PEER) ) {

		errno = EINVAL;
		return -(EXIT_FAILURE);
	}

	sockfd = socket->sd;
	fflag  = 0;

	if ( length > MIN2(MICROTCP_MSS, MIN2(socket->cwnd, socket->sendbuflen)) )
		fflag = 0;
	else
		fflag = 1;

	_timeout(sockfd, TIOUT_ENABLE);

	while ( length ) {

		tmp = MIN2(socket->cwnd, socket->sendbuflen);
		bytes_to_send = MIN2(length, tmp);
		chunks = bytes_to_send / MICROTCP_MSS;  // avoid IP-Fragmentation (break into fragments)

		LOG_DEBUG("\n\e[1mlength = %lu\e[0m\n"
					" > chunks = %lu\n"
					" > bytes_to_send = %lu\n", length, chunks, bytes_to_send);

		for ( index = 0UL; index < chunks; ++index ) {

			tmp = (uint64_t)(buffer) + (index * MICROTCP_MSS);  // pointer arithmetic

			_preapre_send_tcph(socket, &tcph, ( !fflag ) ? (fflag = FRAGMENT) : CTRL_XXX, (void *)(tmp), MICROTCP_MSS);
			memcpy(tbuff, &tcph, MICROTCP_HEADER_SIZE);
			memcpy(tbuff + MICROTCP_HEADER_SIZE, (void *)(tmp), MICROTCP_MSS);

			check( send(sockfd, tbuff, MICROTCP_MSS + MICROTCP_HEADER_SIZE, 0) );
			socket->seq_number += MICROTCP_MSS;

			/** TODO: congestion control - flow control */
		}

		length -= bytes_to_send;
		bytes_to_send -= index * MICROTCP_MSS;

		// semi-filled chunk
		if ( bytes_to_send ) {

			tmp = (uint64_t)(buffer) + (index * MICROTCP_MSS);  // pointer arithmetic

			_preapre_send_tcph(socket, &tcph, ( !length && chunks) ? FRAGMENT : CTRL_XXX, (void *)(tmp), bytes_to_send);
			memcpy(tbuff, &tcph, MICROTCP_HEADER_SIZE);
			memcpy(tbuff + MICROTCP_HEADER_SIZE, (void *)(tmp), bytes_to_send);

			check( send(sockfd, tbuff, bytes_to_send + MICROTCP_HEADER_SIZE, 0) );

			socket->seq_number += bytes_to_send;
			++chunks;
		}

		for ( index = 0UL; index < chunks; ++index ) {

			int64_t ret;

			/** TODO: detect and handle triple-dupACK */

			ret = recv(sockfd, &tcph, MICROTCP_HEADER_SIZE, 0);
			_ntoh_recvd_tcph(tcph);

			// TIMEOUT occured
			if ( (ret < 0) && (errno == EAGAIN) ) {

				/** TODO: implement congestion algorithm (cwnd, ssthresh, etc) */
				socket->ssthresh  = socket->cwnd/2; 
				socket->cwnd      = MICROTCP_MSS;
				socket->dupackcnt = 0U;
				socket->state     = SLOW_START;

				LOG_DEBUG("timeout-occured, retransmiting packet\n");
				// check(-1);
				return microtcp_send(socket,buffer,lengthcpy,flags);
			}

			//Check recieved packet
			if( tcph.control & CTRL_ACK ){//ACK recieved
	
				if( tcph.ack_number == socket->seq_number){//Check ACK and current seq#
	
					LOG_DEBUG("SUCCESFULLY RECIEVED ACK: %d",tcph.ack_number);
					socket->dupackcnt=0U;
					if(socket->cwnd>=socket->ssthresh);

				}else if(tcph.ack_number < socket->seq_number){//if ACK<seq# (dup ack)
	
					socket->dupackcnt++;
					LOG_DEBUG("FALSE ACK RECIEVED (ACK: %d,SEQ: %ld) d.a.c=%d",tcph.ack_number,socket->seq_number,socket->dupackcnt);

					if(socket->dupackcnt>=3){//Enter "fast recovery" mode
						LOG_DEBUG("TRIPLE DUPLICATE ACK DETECTED!");
						socket->ssthresh = socket->cwnd / 2;
						socket->cwnd     = socket->ssthresh + 3;

						//Retransmit the packet
						return microtcp_send(socket,buffer,lengthcpy,flags);
					}
				}
			}
		}
	}

	_timeout(sockfd, TIOUT_DISABLE);


	return EXIT_SUCCESS;
}

ssize_t microtcp_recv(microtcp_sock_t * __restrict__ socket, void * __restrict__ buffer, size_t length, int flags)
{
	microtcp_header_t tcph;
	uint8_t tbuff[MICROTCP_MSS + MICROTCP_HEADER_SIZE]; // c99 and onwards

	int sockfd;
	int frag;

	int64_t total_bytes_read;
	int64_t bytes_read;


	if ( !socket ) {

		errno = EINVAL;
		return -(EXIT_FAILURE);
	}

	if ( (socket->state == INVALID) || (socket->state >= CLOSING_BY_PEER) ) {

		errno = EINVAL;
		return -(EXIT_FAILURE);
	}
	

	total_bytes_read = 0L;
	sockfd = socket->sd;

	check( total_bytes_read = recv(sockfd, tbuff, MICROTCP_MSS + MICROTCP_HEADER_SIZE, 0) );
	memcpy(&tcph, tbuff, MICROTCP_HEADER_SIZE);
	print_tcp_header(socket, &tcph);
	_ntoh_recvd_tcph(tcph);

	if ( tcph.control & CTRL_FIN ) {

		microtcp_shutdown(socket, SHUTDOWN_SERVER);
		return -1L;
	}

	if ( !tcph.data_len )  // zero length packet
		return 0L;


	memcpy(buffer, tbuff + MICROTCP_HEADER_SIZE, tcph.data_len);

	total_bytes_read -= MICROTCP_HEADER_SIZE;
	socket->ack_number += tcph.data_len;
	tcph.seq_number = socket->seq_number;
	tcph.ack_number = socket->ack_number;
	frag = tcph.control & FRAGMENT;

	_preapre_send_tcph(socket, &tcph, CTRL_ACK, NULL, 0U);
	check( send(sockfd, &tcph, MICROTCP_HEADER_SIZE, 0) );

	if ( !frag )  // no fragmentation case
		return total_bytes_read;

	// fragmentation case
	tbuff[total_bytes_read - 1L];
	bytes_read = total_bytes_read;

	do {

		tbuff[bytes_read - 1L] = 0;

		check( bytes_read = recv(sockfd, tbuff, MICROTCP_MSS + MICROTCP_HEADER_SIZE, 0) );
		memcpy(&tcph, tbuff, MICROTCP_HEADER_SIZE);
		// print_tcp_header(socket, &tcph);
		_ntoh_recvd_tcph(tcph);
		memcpy(buffer + total_bytes_read, tbuff + MICROTCP_HEADER_SIZE, tcph.data_len);

		total_bytes_read += bytes_read - MICROTCP_HEADER_SIZE;
		socket->ack_number += tcph.data_len;
		tcph.seq_number = socket->seq_number;
		tcph.ack_number = socket->ack_number;
		frag = tcph.control & FRAGMENT;

		_preapre_send_tcph(socket, &tcph, CTRL_ACK, NULL, 0U);
		check( send(sockfd, &tcph, MICROTCP_HEADER_SIZE, 0) );

	} while ( !frag );


	/** TODO: Flow Control */
	/** TODO: Packet reordering */
	/** TODO: Acknowledgements */

	return total_bytes_read;
}

