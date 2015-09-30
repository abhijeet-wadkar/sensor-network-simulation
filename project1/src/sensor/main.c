/*
 * sensor.c
 *
 *  Created on: Sep 22, 2015
 *      Author: Abhijeet Wadkar
 */
#include <stdio.h>
#include <string.h>
#include <malloc.h>

#include "common.h"
#include "sensor.h"
#include "logger.h"
#include "string_helper_functions.h"

#define TOKEN_MAX 10

int main(int argc, char*argv[])
{
	char *conf_file_name = NULL;
	FILE *conf_file_pointer = NULL;
	char line[LINE_MAX] = {'\0'};
	char *tokens[TOKEN_MAX] = {NULL};
	int count = 0;
	sensor_create_params sensor_device = {NULL, NULL, NULL};
	sensor_handle sensor = NULL;

	LOG(("Number of arguments are: %d\n", argc));

	if(argc<2)
	{
		printf("Please provide configuration file name\n");
		return (0);
	}

	conf_file_name = argv[1];

	LOG(("Configuration File Name is %s\n", conf_file_name));

	conf_file_pointer = fopen(conf_file_name, "r");
	if(!conf_file_pointer)
	{
		printf("Error in opening configuration file\n");
		return (0);
	}

	/* Read line */
	if(fgets(line, LINE_MAX, conf_file_pointer) == NULL)
	{
		LOG(("Cleanup and return"));
		fclose(conf_file_pointer);
		printf("Wrong configuration file\n");
		return (0);
	}
	str_tokenize(line, ":\n\r", tokens, &count);
	if(count<2)
	{
		printf("Wrong configuration file\n");
		fclose(conf_file_pointer);
		return (0);
	}

	str_copy(&sensor_device.gateway_ip_address, tokens[0]);
	str_copy(&sensor_device.gateway_port_no, tokens[1]);
	LOG(("IP Address: %s\n", sensor_device.gateway_ip_address));
	LOG(("Port No: %s\n", sensor_device.gateway_port_no));

	/* Read line */
	if (fgets(line, LINE_MAX, conf_file_pointer) == NULL)
	{
		LOG(("Cleanup and return"));
		fclose(conf_file_pointer);
		printf("Wrong configuration file\n");
		return (0);
	}
	str_tokenize(line, ":\r\n", tokens, &count);
	if (count < 4)
	{
		printf("Wrong configuration file\n");
		fclose(conf_file_pointer);
		return (0);
	}
	if(strcmp("sensor", tokens[0])!=0)
	{
		printf("Wrong configuration file\n");
		fclose(conf_file_pointer);
		return (0);
	}
	str_copy(&sensor_device.sensor_ip_address, tokens[1]);
	str_copy(&sensor_device.sensor_port_no, tokens[2]);
	str_copy(&sensor_device.sensor_area_id, tokens[3]);

	LOG(("ip_address: %s\n", sensor_device.sensor_ip_address));
	LOG(("port_no: %s\n", sensor_device.sensor_port_no));
	LOG(("area_id: %s\n", sensor_device.sensor_area_id));

	create_sensor(&sensor, &sensor_device);
	delete_sensor(sensor);

	free(sensor_device.gateway_ip_address);
	free(sensor_device.gateway_port_no);
	free(sensor_device.sensor_area_id);
	free(sensor_device.sensor_ip_address);
	free(sensor_device.sensor_port_no);
	fclose(conf_file_pointer);
	return (0);
}
