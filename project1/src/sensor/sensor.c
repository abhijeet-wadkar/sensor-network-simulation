/*
 * sensor.c
 *
 *  Created on: Sep 27, 2015
 *      Author: abhijeet
 */

#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <ctype.h>

#include "common.h"
#include "sensor.h"
#include "error_codes.h"
#include "logger.h"
#include "network_functions.h"
#include "message.h"
#include "string_helper_functions.h"

static void* read_callback(void *context);
static void* set_value_thread(void *context);
void sighand(int signo);

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
	sensor->clock = 0;
	sensor->value = 0;
	sensor->run = 1;

	sensor->sensor_value_file_pointer = fopen(params->sensor_value_file_name, "r");
	if(!sensor->sensor_value_file_pointer)
	{
		delete_sensor(sensor);
		return (E_FAILURE);
	}

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

	struct sigaction        actions;
	memset(&actions, 0, sizeof(actions));
	sigemptyset(&actions.sa_mask);
	actions.sa_flags = 0;
	actions.sa_handler = sighand;
	sigaction(SIGALRM,&actions,NULL);
	pthread_create(&sensor->set_value_thread, NULL, &set_value_thread, sensor);

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
		if(sensor->set_value_thread)
		{
			sensor->run = 0;
			pthread_kill(sensor->set_value_thread, SIGALRM);
			pthread_join(sensor->set_value_thread, NULL);
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
	case SET_INTERVAL:
		printf("SetInterval message received");
		sensor->interval = msg.u.value;
		break;
	default:
		printf("Other message was received\n");
		break;
	}

	return NULL;
}

void sighand(int signo)
{
	LOG(("EXITING SET_VALUE_THREAD\n"));
}

void* set_value_thread(void *context)
{
	sensor_context *sensor = NULL;
	message msg;
	int return_value;
	char *tokens[10];
	char line[LINE_MAX];
	int count = 0;
	int start, end, value;

	sensor = (sensor_context*)context;

	msg.type = CURRENT_VALUE;
	while(sensor->run)
	{
		if(!(start <= sensor->clock && sensor->clock < end))
		{
			/* Figure out the value from file */
			if(fgets(line, LINE_MAX, sensor->sensor_value_file_pointer) == NULL)
			{
				LOG(("Seeking to beginning of file"));
				rewind(sensor->sensor_value_file_pointer);
				sensor->clock = 0;
				continue;
			}

			str_tokenize(line, ";\n\r", tokens, &count);
			if(count != 3)
			{
				LOG(("Count: %d, line:%s\n", count, line));
				LOG(("Wrong sensor value file\n"));
			}

			start = atoi(tokens[0]);
			end = atoi(tokens[1]);
			value = atoi(tokens[2]);
			sensor->value = value;
		}

		msg.u.value = sensor->value;

		return_value = write_message(sensor->socket_fd, &msg);
		if(E_SUCCESS != return_value)
		{
			LOG(("Error in sending sensor value to gateway\n"));
		}
		LOG(("Sleeping for %d second(s)\n", sensor->interval));
		sleep(sensor->interval);
		sensor->clock += sensor->interval;
	}

	LOG(("Exiting SetValueThread...\n"));
	return (NULL);
}
