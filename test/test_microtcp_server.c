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

#include <netinet/ip.h>
#include <stdio.h>
#include <stdlib.h>

#define STR_ERROR "\033[1;31mERROR\033[93m:\033[0m"

void check_main_input(int argc, char ** argv);

int main(int argc, char ** argv)
{
    microtcp_sock_t ssock;
    struct sockaddr_in addr;
    uint16_t port;


    check_main_input(argc, argv);
    printf("initializing server...\n");
    ssock = microtcp_socket(AF_INET, SOCK_DGRAM, 0);

    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port        = htons(port);
    addr.sin_family      = AF_INET;

    microtcp_bind(&ssock, (struct sockaddr *)(&addr), sizeof(addr));
    

    close(ssock.sd);

    return 0;
}

void check_main_input(int argc, char ** argv) {

    if ( argc < 2 ) {

        printf(STR_ERROR " unspecified port number\n");
        printf("./test_microtcp_server <port-number>\n");
        exit(EXIT_FAILURE);
    }
}
