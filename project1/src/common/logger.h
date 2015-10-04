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

#ifdef DEBUG
#define LOG(x) log_to_file x;
#else
#define LOG
#endif

void log_to_file(char *msg, ...);

#endif /* LOGGER_H_ */
