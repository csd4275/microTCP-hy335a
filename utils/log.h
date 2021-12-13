/*
 * microtcp, a lightweight implementation of TCP for teaching,
 * and academic purposes.
 *
 * Copyright (C) 2017  Manolis Surligas <surligas@gmail.com>
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

#ifndef UTILS_LOG_H_
#define UTILS_LOG_H_

#include <stdio.h>
#include <sys/syscall.h>


#define ENABLE_DEBUG_MSG


#ifdef ENABLE_DEBUG_MSG
#define LOG_INFO(M, ...)                                                        \
                fprintf(stderr, "[INFO]: %s:%d: " M "\n", __FILE__, __LINE__, ##__VA_ARGS__)
#else
#define LOG_INFO(M, ...)
#endif

#define LOG_ERROR(M, ...)                                                       \
        fprintf(stderr, "[ERROR] %s:%d: " M "\n", __FILE__, __LINE__, ##__VA_ARGS__)

#define LOG_WARN(M, ...)                                                                \
        fprintf(stderr, "[WARNING] %s:%d: " M "\n", __FILE__, __LINE__, ##__VA_ARGS__)

////////////////////////////////////////////////////////////
#ifdef ENABLE_DEBUG_MSG

#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "../lib/microtcp.h"

int seqbase;

void strctrl(uint16_t cbits){

	if ( cbits & CTRL_FIN )
		printf("[\033[94mFIN\033[0m]");

	if ( cbits & CTRL_SYN )
		printf("[\033[94mSYN\033[0m]");

	if ( cbits & CTRL_RST )
		printf("[\033[94mRST\033[0m]");

	if ( cbits & CTRL_ACK )
		printf("[\033[94mACK\033[0m]");

	printf("\n");
}

/**
 * @brief Print the microTCP header (host-byte-order)
 * 
 * @param tcph header must be in network byte order
 */
void print_tcp_header(microtcp_header_t * tcph){

	static int counter = 0;

	/** TODO: future_use{0, 1, 2} */

    if ( !counter )
        seqbase = ntohl(tcph->seq_number);

    printf("\n\033[1mTCP-header#%d\033[0m\n", counter);
    printf("  * \033[4mseq#\033[0m = \033[3m%d\033[0m\n", ntohl(tcph->seq_number) - seqbase);
    printf("  * \033[4mack#\033[0m = %d\n", ntohl(tcph->ack_number));
    printf("  * \033[4mctrl\033[0m = %d - ", ntohs(tcph->control));
	strctrl(ntohs(tcph->control));
    printf("  * \033[4mwind\033[0m = %d\n", ntohs(tcph->window));
    printf("  * \033[4mdata\033[0m = %d\n", ntohl(tcph->data_len));
    printf("  * \033[4mcsum\033[0m = %d\n\n", ntohl(tcph->checksum));

	++counter;
}

#define __FILENAME__   \
        (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)


#define check(x) _check(x, __LINE__, __FUNCTION__);

static inline void _check(int retval, int line, const char * funct){

    if ( retval < 0 ) {

		#ifdef ENABLE_DEBUG_MSG
        printf("\033[93m%d\033[0m::\033[0;91m%s\033[0m() failed: \033[4m%s\033[0m\n", line, funct, strerror(errno));
        #endif
        exit(EXIT_FAILURE);
    }
}
#define LOG_DEBUG(M, ...)                                                       \
        fprintf(stderr, "\033[1m[\033[0;31mDEBUG\033[0;1m]\033[0m: \033[93m%s\033[0m::\033[93m%s\033[0m::\033[93m%d\033[0m -> " M "\n", __FILENAME__ , __FUNCTION__, __LINE__, ##__VA_ARGS__)
#else
#define LOG_DEBUG(M, ...)
#define check(x)
void print_tcp_header(microtcp_header_t * tcph){}
#endif

#endif /* UTILS_LOG_H_ */
