/*
 * sensor.h
 *
 *  Created on: Sep 22, 2015
 *      Author: Abhijeet Wadkar
 */

#ifndef SENSOR_H_
#define SENSOR_H_

#include "network_read_thread.h"

typedef void* sensor_handle;

typedef struct sensor_create_params
{
	char *sensor_ip_address;
	char *sensor_port_no;
	char *sensor_area_id;
	char *gateway_ip_address;
	char *gateway_port_no;
}sensor_create_params;

typedef struct sensor_context
{
	sensor_create_params *sensor_params;
	int interval;
	int socket_fd;
	network_thread_handle network_thread;
}sensor_context;

int create_sensor(sensor_handle *handle, sensor_create_params *params);
void delete_sensor(sensor_handle);


#endif /* SENSOR_H_ */