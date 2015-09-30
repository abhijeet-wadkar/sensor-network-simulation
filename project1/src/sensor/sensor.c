/*
 * sensor.c
 *
 *  Created on: Sep 27, 2015
 *      Author: abhijeet
 */

#include <stdio.h>
#include <malloc.h>
#include <string.h>
#include <unistd.h>

#include "sensor.h"
#include "error_codes.h"
#include "logger.h"
#include "network_functions.h"

static void* read_callback(void *context);

int create_sensor(sensor_handle *handle, sensor_create_params *params)
{
	sensor_context *sensor = NULL;
	int return_value = E_FAILURE;
	int index = 100;

	sensor = (sensor_context*)malloc(sizeof(sensor_context));
	if(NULL == sensor)
	{
		delete_sensor((sensor_handle)sensor);
		return (E_OUT_OF_MEMORY);
	}

	memset(sensor, 0, sizeof(sensor_context));
	sensor->interval = 5;

	/* create network read thread */
	return_value = create_network_thread(&sensor->network_thread, params->sensor_ip_address);
	if(E_SUCCESS != return_value)
	{
		LOG(("Error in creating n/w read thread"));
		delete_sensor((sensor_handle)sensor);
		return (return_value);
	}

	/* create connection to server */
	return_value = create_socket(&sensor->socket_fd, params->gateway_ip_address, params->gateway_port_no);
	if(E_SUCCESS != return_value)
	{
		LOG(("Connection to Server failed\n"));
		delete_sensor((sensor_handle)sensor);
		return (return_value);
	}

	/* add socket to network read thread */
	return_value = add_socket(sensor->network_thread, sensor->socket_fd,  (void*)sensor, &read_callback);
	if(E_SUCCESS != return_value)
	{
		LOG(("Connection to Server failed\n"));
		delete_sensor((sensor_handle)sensor);
		return (return_value);
	}

	/* send sensor data periodically */
	/*TODO*/
	for(index=0; index<1; index++)
	{
		send_socket(sensor->socket_fd, "Hello, How are you?", 20);
	}
	*handle = sensor;
	return (E_SUCCESS);
}

void delete_sensor(sensor_handle handle)
{
	/* release all the resources */
	sensor_context* sensor = (sensor_context*)handle;

	if(sensor)
	{
		if(sensor->network_thread)
		{
			delete_network_thread(sensor->network_thread);
		}
		if(sensor->socket_fd)
		{
			close_socket(sensor->socket_fd);
		}

		free(sensor);
	}
}

static void* read_callback(void *context)
{
	sensor_context *sensor = (sensor_context*)context;
	int read_count;
	char buffer[100] = {'\0'};

	read_count = read(sensor->socket_fd, &buffer, 100);
	if(read_count < 0)
	{
		LOG(("Error in read"));
	}
	LOG(("DATA Read from socket: %s", buffer));
	return NULL;
}
