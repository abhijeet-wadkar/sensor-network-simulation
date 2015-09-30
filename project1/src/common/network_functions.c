/*
 * network_functions.c
 *
 *  Created on: Sep 27, 2015
 *      Author: abhijeet
 */

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <stdlib.h>
#include <arpa/inet.h>

#include "network_functions.h"
#include "error_codes.h"
#include "logger.h"

int create_socket(int *socket_fd, char *ip_address, char* port_no)
{
	struct sockaddr_in serv_addr;

	*socket_fd = socket(AF_INET, SOCK_STREAM, 0);
	if(*socket_fd < 0)
	{
		LOG(("Create socket error\n"));
		return (E_SOCKET_CREATE_ERROR);
	}

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(atoi(port_no));

	if(inet_pton(AF_INET, ip_address, &serv_addr.sin_addr) <= 0)
	{
		LOG(("inet_pton() failed\n"));
		return (E_FAILURE);
	}

	if(connect(*socket_fd, (struct sockaddr *)&serv_addr, sizeof(serv_addr))	< 0)
	{
		LOG(("Connect Failed\n"));
		return (E_FAILURE);
	}
	return (E_SUCCESS);
}

void close_socket(int socket_fd)
{
	close(socket_fd);
	socket_fd = -1;
	return;
}

void send_socket(int socket_fd, char* data, int length)
{
	int send_count = 0;

	send_count = write(socket_fd, data, length);
	if(send_count < 0)
	{
		LOG(("Error in sending data\n"));
	}
}
