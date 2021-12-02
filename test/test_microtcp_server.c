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
 * You can use this file to write a test microTCP server.
 * This file is already inserted at the build system.
 */


#include "../lib/microtcp.h"
#include "../utils/errorc.h"
#include "connections.h"

#include <netinet/ip.h>
#include <stdio.h>
#include <stdlib.h>

void check_main_input(int argc, char ** argv);

int main(int argc, char ** argv)
{
    microtcp_sock_t ssock;
    struct sockaddr_in addr;
    uint16_t port;
    microtcp_header_t recv_header;

    server_check_main_input(argc, argv);
    printf("initializing server...\n");

    ssock = microtcp_socket(AF_INET, SOCK_DGRAM, 0);

    if ( ssock.sd > 0 )
        printf("socket created\n");
    else
        exit(EXIT_FAILURE);

    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port        = htons(atoi(argv[1]));
    addr.sin_family      = AF_INET;

    check(microtcp_bind(&ssock, (struct sockaddr *)(&addr), sizeof(addr)));
    check(microtcp_accept(&ssock, &addr, sizeof(addr)));

    while(1){
        printf("Waiting to receive packet from client...\n");
        recvfrom(ssock.sd,(void*)&recv_header,sizeof(recv_header),0,NULL,NULL);
        printf("Received packet from client!\n");
        if(recv_header.control==htons(CTRL_FIN|CTRL_ACK)){
            printf("Recieved FIN-ACK from client, calling SHUT_RD\n");
            microtcp_shutdown(&ssock,SHUT_RD);
            break;    
        }
    }

    printf("Connection with host successfully closed!\n");
    return 0;
}