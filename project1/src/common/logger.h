/*
 * logger.h
 *
 *  Created on: Sep 22, 2015
 *      Author: abhijeet
 */

#ifndef LOGGER_H_
#define LOGGER_H_

#include <stdio.h>

#ifdef DEBUG
#define LOG(x) printf("LOGGER:%d ",__LINE__); printf x
#else
#define LOG
#endif

#endif /* LOGGER_H_ */
