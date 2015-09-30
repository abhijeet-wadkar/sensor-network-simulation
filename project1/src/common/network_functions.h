/*
 * network_functions.h
 *
 *  Created on: Sep 27, 2015
 *      Author: abhijeet
 */

int create_socket(int *socket_fd, char *ip_address, char* port_no);
void close_socket(int socket_fd);
void send_socket(int socket_fd, char* data, int length);


