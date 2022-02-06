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

/*
 * You can use this file to write a test microTCP client.
 * This file is already inserted at the build system.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include "connections.h"
#include "../lib/microtcp.h"
#include "../utils/log.h"

#define TEST_BYTES 2805

void send_file(FILE *fp, microtcp_sock_t sockfp);


int main(int argc, char **argv) {
    int flag = atoi(argv[3]);

    microtcp_sock_t csock;
    struct sockaddr_in addr;
    uint16_t port=atoi(argv[2]);
    uint32_t iaddr;
    char buff[32];
    void *frag_test;


    client_check_main_input(argc, argv);
    csock = microtcp_socket(AF_INET, SOCK_DGRAM, 0);

    inet_pton(AF_INET, argv[1], &iaddr);
    addr.sin_addr.s_addr = iaddr;
    addr.sin_port        = htons(port);
    addr.sin_family      = AF_INET;

    microtcp_connect(&csock,(struct sockaddr*)&addr,sizeof(addr));

    if(flag == 1) {
        
        FILE* fp = fopen(argv[4], "r");
        
        // if ( fd  < 0 ) {

        //     perror("open() failed");
        //     exit(EXIT_FAILURE);
        // }

        if ( !(frag_test = malloc(TEST_BYTES + 1)) ) {

            perror("malloc() failed");
            exit(EXIT_FAILURE);
        }

        // read(fd, frag_test, TEST_BYTES);
        // *(char *)(frag_test + TEST_BYTES) = 0;
    
        check( microtcp_send(&csock, "Pousth Bisia!!!", 16UL, 0) );
        check( microtcp_send(&csock, "Papastamo GAmiesai!1!!1!", 25UL, 0) );
        send_file(fp, csock);
        
        // check( microtcp_send(&csock, frag_test, TEST_BYTES, 0) );
    }
    else if(flag == 2) {

    }
    else if(flag == 3) {

    }

    LOG_DEBUG("Shutting down the connection.\n");
    microtcp_shutdown(&csock,SHUTDOWN_CLIENT);

    LOG_DEBUG("Connection has been shut down successfully!\n");

    return 0;
}


void send_file(FILE *fp, microtcp_sock_t sockfp) {
    char data[1024] = {0};

    while(fgets(data, 1024, fp) != NULL) {
        microtcp_send(&sockfp, data, 1024, 0);
    }
    bzero(data, 1024);
}