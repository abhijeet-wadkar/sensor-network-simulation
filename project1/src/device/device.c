/*
 * device.c
 *
 *  Created on: Oct 1, 2015
 *      Author: Abhijeet Wadkar
 */

#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <string.h>
#include <unistd.h>

#include "device.h"
#include "error_codes.h"
#include "logger.h"
#include "network_functions.h"
#include "message.h"

static void* read_callback(void *context);

int create_device(device_handle *handle, device_create_params *params)
{
	device_context *device = NULL;
	int return_value = E_FAILURE;

	device = (device_context*)malloc(sizeof(device_context));
	if(NULL == device)
	{
		delete_device((device_handle)device);
		return (E_OUT_OF_MEMORY);
	}

	memset(device, 0, sizeof(device_context));
	device->device_params = params;
	device->state = 0;

	/* create network read thread */
	return_value = create_network_thread(&device->network_thread, params->device_ip_address);
	if(E_SUCCESS != return_value)
	{
		LOG(("Error in creating n/w read thread"));
		delete_device((device_handle)device);
		return (return_value);
	}

	/* create connection to server */
	return_value = create_socket(&device->socket_fd, params->gateway_ip_address, params->gateway_port_no);
	if(E_SUCCESS != return_value)
	{
		LOG(("Connection to Server failed\n"));
		delete_device((device_handle)device);
		return (return_value);
	}

	/* add socket to network read thread */
	return_value = add_socket(device->network_thread, device->socket_fd,  (void*)device, &read_callback);
	if(E_SUCCESS != return_value)
	{
		LOG(("Connection to Server failed\n"));
		delete_device((device_handle)device);
		return (return_value);
	}

	message msg;

	/* register device with gateway */
	msg.type = REGISTER;
	msg.u.s.type = SMART_DEVICE;
	msg.u.s.ip_address = device->device_params->device_ip_address;
	msg.u.s.port_no = device->device_params->device_port_no;
	msg.u.s.area_id = device->device_params->device_area_id;

	return_value = write_message(device->socket_fd, &msg);
	if(E_SUCCESS != return_value)
	{
		LOG(("Error in registering device\n"));
		return (E_FAILURE);
	}

	*handle = device;
	return (E_SUCCESS);
}

void delete_device(device_handle handle)
{
	/* release all the resources */
	device_context* device = (device_context*)handle;

	if(device)
	{
		if(device->network_thread)
		{
			delete_network_thread(device->network_thread);
		}
		if(device->socket_fd)
		{
			close_socket(device->socket_fd);
		}

		free(device);
	}
}

static void* read_callback(void *context)
{
	device_context *device = (device_context*)context;
	int return_value = 0;
	message msg;
	message snd_msg;

	return_value = read_message(device->socket_fd, &msg);
	if(return_value != E_SUCCESS)
	{
		if(return_value == E_SOCKET_CONNECTION_CLOSED)
		{
			printf("Socket connection from server closed...\n");
			exit(0);
		}
		LOG(("Error in read message\n"));
		return NULL;
	}

	switch(msg.type)
	{
	case SWITCH:
		LOG(("SWITCH: %d\n", msg.u.value));

		if(msg.u.value != device->state)
			device->state = msg.u.value;

		snd_msg.type = CURRENT_STATE;
		snd_msg.u.value = device->state;
		return_value = write_message(device->socket_fd, &snd_msg);
		if(E_SUCCESS != return_value)
		{
			LOG(("Error in sending message to gateway\n"));
		}
		break;
	default:
		LOG(("Other message was received\n"));
		break;
	}

	return NULL;
}
