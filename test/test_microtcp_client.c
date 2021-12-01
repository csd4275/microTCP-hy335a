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
#include <connections.h>
#include <../lib/microtcp.h>

int
main(int argc, char **argv)
{
    microtcp_sock_t csock;
    struct sockaddr_in addr;
    uint16_t port=atoi(argv[2]);


    client_check_main_input(argc, argv);
    csock = microtcp_socket(AF_INET, SOCK_DGRAM, 0);

    addr.sin_addr.s_addr = htonl(argv[1]);
    addr.sin_port        = htons(port);
    addr.sin_family      = AF_INET;

    printf("Connecting to server %d...\n",addr);
    microtcp_connect(&csock,(struct sockaddr*)&addr,sizeof(addr));

    printf("Sucessfully connected to server!\n");
    close(csock.sd);

    return 0;
}
