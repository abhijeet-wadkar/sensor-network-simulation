/*
 * gateway.c
 *
 *  Created on: Oct 1, 2015
 *      Author: Abhijeet Wadkar, Devendra Dahiphale
 */
#include <stdio.h>
#include <malloc.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>

#include "gateway.h"
#include "error_codes.h"
#include "logger.h"
#include "network_functions.h"
#include "network_read_thread.h"

char* device_string[] = {
		"sensor",
		"device",
		"gateway",
		"unknown"};

char* state_string[] = {
		"off",
		"on"
};

void* accept_callback(void *context);
void* read_callback(void*);

void print_state(gateway_context *gateway)
{
	int index;

	LOG_GATEWAY(("-----------------------------------------------\n"));
	for(index=0; index<gateway->client_count; index++)
	{
		gateway_client *client = gateway->clients[index];
		if(client->type == SMART_DEVICE)
		{
			LOG_GATEWAY(("%d----%s:%s----%s----%s----%s\n",
									(int)(client&&0xFFFF),
									client->client_ip_address,
									client->client_port_number,
									device_string[client->type],
									client->area_id,
									state_string[client->state]));
		}
		else
		{
			LOG_GATEWAY(("%d----%s:%s----%s----%s----%d\n",
							(int)(client&&0xFFFF),
							client->client_ip_address,
							client->client_port_number,
							device_string[client->type],
							client->area_id,
							client->value));
		}
	}
	LOG_GATEWAY(("-----------------------------------------------\n"));
}

int create_gateway(gateway_handle* handle, gateway_create_params *params)
{
	gateway_context *gateway = NULL;
	int return_value = 0;

	gateway = (gateway_context*)malloc(sizeof(gateway_context));
	if(NULL == gateway)
	{
		LOG_ERROR(("ERROR: Out of memory\n"));
		return (E_OUT_OF_MEMORY);
	}

	gateway->client_count = 0;

	/* create network read thread */
	return_value = create_network_thread(&gateway->network_thread, params->gateway_ip_address);
	if(E_SUCCESS != return_value)
	{
		LOG_ERROR(("ERROR: Error in creating n/w read thread\n"));
		delete_gateway((gateway_handle)gateway);
		return (return_value);
	}

	/* create connection to server */
	return_value = create_server_socket(&gateway->server_socket_fd, params->gateway_ip_address, params->gateway_port_no);
	if(E_SUCCESS != return_value)
	{
		LOG_ERROR(("ERROR: Error in creating the socket\n"));
		delete_gateway((gateway_handle)gateway);
		return (return_value);
	}

	/* add socket to network read thread */
	return_value = add_socket(gateway->network_thread, gateway->server_socket_fd,  (void*)gateway, &accept_callback);
	if(E_SUCCESS != return_value)
	{
		LOG_ERROR(("ERROR: add_socket() failed\n"));
		delete_gateway((gateway_handle)gateway);
		return (return_value);
	}
	*handle = gateway;

	return (E_SUCCESS);
}
void delete_gateway(gateway_handle handle)
{
	/* release all the resources */
	gateway_context* gateway = (gateway_context*)handle;
	int index;

	if(gateway)
	{
		for(index=0; index<gateway->client_count; index++)
		{
			remove_socket(gateway->network_thread, gateway->clients[index]->comm_socket_fd);
			if(gateway->clients[index]->client_ip_address)
				free(gateway->clients[index]->client_ip_address);
			if(gateway->clients[index]->client_port_number)
				free(gateway->clients[index]->client_port_number);
			if(gateway->clients[index]->area_id)
				free(gateway->clients[index]->area_id);
			free(gateway->clients[index]);
		}

		if(gateway->network_thread)
		{
			delete_network_thread(gateway->network_thread);
		}
		if(gateway->server_socket_fd)
		{
			close_socket(gateway->server_socket_fd);
		}

		free(gateway);
	}
}

void* accept_callback(void *context)
{
	int return_value = 0;
	gateway_context *gateway = NULL;
	gateway_client *client = NULL;

	gateway = (gateway_context*)context;

	client = (gateway_client*)malloc(sizeof(gateway_client));
	if(!client)
	{
		LOG_DEBUG(("DEBUG: Out of memory\n"));
		return (NULL);
	}

	client->gateway = context;
	client->comm_socket_fd = accept(gateway->server_socket_fd, (struct sockaddr*)NULL, NULL);
	if(client->comm_socket_fd < 0)
	{
		LOG_ERROR(("ERROR: Accept call failed\n"));
		free(client);
		return NULL;
	}

	gateway->clients[gateway->client_count] = client;
	gateway->client_count++;

	/* add socket to network read thread */
	return_value = add_socket(gateway->network_thread, client->comm_socket_fd,  (void*)client, &read_callback);
	if(E_SUCCESS != return_value)
	{
		LOG_ERROR(("ERROR: add_socket() failed\n"));
		free(client);
		return (NULL);
	}
	client->connection_state = 1;

	return (NULL);
}

void* read_callback(void *context)
{
	gateway_client *client = (gateway_client*)context;
	gateway_context *gateway = client->gateway;
	int return_value = 0;
	message msg;
	message snd_msg;
	int index;
	int flag_found = 0;

	return_value = read_message(client->comm_socket_fd, &msg);
	if(return_value != E_SUCCESS)
	{
		if(return_value == E_SOCKET_CONNECTION_CLOSED)
		{
			LOG_ERROR(("ERROR: Connection closed for client: %s-%s-%s...\n",
					client->client_ip_address,
					client->client_port_number,
					client->area_id));
			remove_socket(client->gateway->network_thread, client->comm_socket_fd);
			client->connection_state = 0;
			LOG_ERROR(("ERROR: Connection closed for client\n"));
			index = 0;
			flag_found = 0;
			for(index=0; index<gateway->client_count; index++)
			{
				if(gateway->clients[index]->comm_socket_fd == client->comm_socket_fd)
				{
					flag_found = 1;
					break;
				}
			}
			if(flag_found == 1)
			{
				for(;index<gateway->client_count-1; index++)
				{
					gateway->clients[index] = gateway->clients[index+1];
				}
				gateway->client_count--;
			}
			if(client->client_ip_address)
				free(client->client_ip_address);
			if(client->client_port_number)
				free(client->client_port_number);
			if(client->area_id)
				free(client->area_id);
			free(client);
			return NULL;
		}
		LOG_ERROR(("ERROR: Error in read message\n"));
		return NULL;
	}

	switch(msg.type)
	{
	case SET_INTERVAL:
		LOG_DEBUG(("DEBUG: SetInterval message received, Value: %d\n", msg.u.value));
		break;
	case REGISTER:
		LOG_DEBUG(("DEBUG: Register message received\n"));
		client->type = msg.u.s.type;
		client->client_ip_address = msg.u.s.ip_address;
		client->client_port_number = msg.u.s.port_no;
		client->area_id = msg.u.s.area_id;
		LOG_DEBUG(("DEBUG: DeviceType:%d\n", client->type));
		LOG_DEBUG(("DEBUG: IP Address: %s\n", client->client_ip_address));
		LOG_DEBUG(("DEBUG: Port Number: %s\n", client->client_port_number));
		LOG_DEBUG(("DEBUG: Area Id: %s\n", client->area_id));
		if(client->type == SENSOR)
		{
			client->state = 1;
		}
		if(client->type == SMART_DEVICE)
		{
			client->state = 0;
		}
		break;
	case CURRENT_VALUE:
		LOG_DEBUG(("Current value message received\n"));
		LOG_DEBUG(("Value: %d\n", msg.u.value));

		client->value = msg.u.value;

		if(msg.u.value < 32)
		{
			/* switch all smart devices in area id on */
			int index;
			for(index=0; index<gateway->client_count; index++)
			{
				if(gateway->clients[index]->type == SMART_DEVICE &&
						strcmp(gateway->clients[index]->area_id, client->area_id)==0 &&
						gateway->clients[index]->state == 0)
				{
					snd_msg.type = SWITCH;
					snd_msg.u.value = 1;
					return_value = write_message(gateway->clients[index]->comm_socket_fd, &snd_msg);
					if(E_SUCCESS != return_value)
					{
						LOG_ERROR(("Error in sending switch on message to device %s-%s-%s",
								gateway->clients[index]->client_ip_address,
								gateway->clients[index]->client_port_number,
								gateway->clients[index]->area_id));
					}
					gateway->clients[index]->state = 1;
				}
			}
		}

		if(msg.u.value > 34)
		{
			/* switch all smart devices in area id off */
			int index;
			int flag = 1;

			for(index=0; index<gateway->client_count; index++)
			{
				if(gateway->clients[index]->type == SENSOR &&
						strcmp(gateway->clients[index]->area_id, client->area_id)==0 &&
						gateway->clients[index]->value < 34)
				{
					flag = 0;
					break;
				}
			}

			if(flag == 1)
			{
				for(index=0; index<gateway->client_count; index++)
				{
					if(gateway->clients[index]->type == SMART_DEVICE &&
							strcmp(gateway->clients[index]->area_id, client->area_id)==0 &&
							gateway->clients[index]->state == 1)
					{
						snd_msg.type = SWITCH;
						snd_msg.u.value = 0;
						return_value = write_message(gateway->clients[index]->comm_socket_fd, &snd_msg);
						if(E_SUCCESS != return_value)
						{
							LOG_ERROR(("Error in sending switch on message to device %s-%s-%s",
									gateway->clients[index]->client_ip_address,
									gateway->clients[index]->client_port_number,
									gateway->clients[index]->area_id));
						}
						gateway->clients[index]->state = 0;
					}
				}
			}
		}
		print_state(client->gateway);
		break;
	case CURRENT_STATE:
		LOG_DEBUG(("DEBUG: Current state message is received\n"));
		//client->state = msg.u.value;
		//print_state(client->gateway);
		break;
	default:
		LOG_DEBUG(("Unknown/Unhandled message is received\n"));
		break;
	}
	return (NULL);
}

int set_interval(gateway_handle handle, int index, int interval)
{
	gateway_context *gateway = handle;
	message snd_msg;

	if(0 <= index && index < gateway->client_count)
	{
		snd_msg.type = SET_INTERVAL;
		snd_msg.u.value = interval;
		return (write_message(gateway->clients[index]->comm_socket_fd, &snd_msg));
	}
	else
	{
		LOG_ERROR(("ERROR: Such sensor don't exists\n"));
	}
	return (E_FAILURE);
}

void print_sensors(gateway_handle handle)
{
	int index;
	gateway_context *gateway = handle;

	printf("----------------List of sensor---------------\n");
	printf("<ID>-<IP_Address>-<Port_Number>-<AreaId>\n");
	for(index=0; index<gateway->client_count; index++)
	{
		if(gateway->clients[index]->type == SENSOR)
		{
			printf("%d:%s-%s-%s\n",
					index,
					gateway->clients[index]->client_ip_address,
					gateway->clients[index]->client_port_number,
					gateway->clients[index]->area_id);
		}
	}
}
