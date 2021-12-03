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
#include <arpa/inet.h>
#include "connections.h"
#include "../lib/microtcp.h"

int
main(int argc, char **argv)
{
    microtcp_sock_t csock;
    struct sockaddr_in addr;
    uint16_t port=atoi(argv[2]);
    uint32_t iaddr;
    char buff[32];


    client_check_main_input(argc, argv);
    csock = microtcp_socket(AF_INET, SOCK_DGRAM, 0);

    printf("argv[1]: %s\n", argv[1]);
    printf("argv[2]: %s\n", argv[2]);
    printf("sockfd : %d\n", csock.sd);

    inet_pton(AF_INET, argv[1], &iaddr);
    addr.sin_addr.s_addr = iaddr;
    addr.sin_port        = htons(port);
    addr.sin_family      = AF_INET;

    inet_ntop(AF_INET, &iaddr, buff, sizeof(struct sockaddr_in));
    LOG_DEBUG("Connecting to server %s [%d]\n",buff, iaddr);
    microtcp_connect(&csock,(struct sockaddr*)&addr,sizeof(addr));

    printf("Sucessfully connected to server!\n\n\n");
    
    printf("calling SHUT_WR on client socket.\n");
    microtcp_shutdown(&csock,SHUT_WR);

    printf("Waiting for FINACK packet from server\n");
    microtcp_header_t finack_recv_header;
    recvfrom(csock.sd,(void*)&finack_recv_header,sizeof(finack_recv_header),0,NULL,NULL);
    
    if( ntohs(finack_recv_header.control) == (CTRL_FIN | CTRL_ACK) ){

        printf("Recieved FIN-ACK from server, calling SHUT_RD\n");
        microtcp_shutdown(&csock,SHUT_RD);
    }

    printf("\nConnection has been shut down successfully!\n");

    return 0;
}
