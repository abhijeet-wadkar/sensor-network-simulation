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
#include "message.h"

static void* read_callback(void *context);
static void* set_interval_thread(void *context);

int create_sensor(sensor_handle *handle, sensor_create_params *params)
{
	sensor_context *sensor = NULL;
	int return_value = E_FAILURE;

	sensor = (sensor_context*)malloc(sizeof(sensor_context));
	if(NULL == sensor)
	{
		delete_sensor((sensor_handle)sensor);
		return (E_OUT_OF_MEMORY);
	}

	memset(sensor, 0, sizeof(sensor_context));
	sensor->interval = 5;
	sensor->sensor_params = params;

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

	message msg;

	/* register sensor with gateway */
	msg.type = REGISTER;
	msg.u.s.type = SENSOR;
	msg.u.s.ip_address = sensor->sensor_params->sensor_ip_address;
	msg.u.s.port_no = sensor->sensor_params->sensor_port_no;
	msg.u.s.area_id = sensor->sensor_params->sensor_area_id;

	return_value = write_message(sensor->socket_fd, &msg);
	if(E_SUCCESS != return_value)
	{
		LOG(("Error in registering sensor\n"));
		return (E_FAILURE);
	}

	pthread_create(&sensor->set_interval_thread, NULL, &set_interval_thread, sensor);

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
	int return_value = 0;
	message msg;

	return_value = read_message(sensor->socket_fd, &msg);
	if(return_value != E_SUCCESS)
	{
		LOG(("Error in read message\n"));
		return NULL;
	}

	switch(msg.type)
	{
	case SET_INTERVAL:
		printf("SetInterval message received");
		break;
	default:
		printf("Other message was received\n");
		break;
	}

	return NULL;
}

void* set_interval_thread(void *context)
{
	sensor_context *sensor = NULL;
	message msg;
	int return_value;

	sensor = (sensor_context*)context;

	msg.type = CURRENT_VALUE;
	while(1)
	{
		/* Figure out the value from file */
		msg.u.value = 30 + rand()%10;
		return_value = write_message(sensor->socket_fd, &msg);
		if(E_SUCCESS != return_value)
		{
			LOG(("Error in sending sensor value to gateway\n"));
		}
		sleep(sensor->interval);
	}

	return (NULL);
}
