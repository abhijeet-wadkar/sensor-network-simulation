/*
 * logger.h
 *
 *  Created on: Sep 22, 2015
 *      Author: abhijeet
 */

#ifndef LOGGER_H_
#define LOGGER_H_

#include <stdio.h>

#define LOG_FILENAME "log.txt"
#define GATEWAY_LOG_FILENAME "gateway_log.txt"

#ifdef DEBUG
#define LOG(x) log_to_file x;
#else
#define LOG(x)
#endif

#define LOG_GATEWAY(x) log_to_gateway_log_file x;

void log_to_file(char *msg, ...);
void log_to_gateway_log_file(char *msg, ...);

#endif /* LOGGER_H_ */
