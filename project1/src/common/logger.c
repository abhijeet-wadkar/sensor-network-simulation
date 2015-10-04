/*
 * logger.c
 *
 *  Created on: Oct 2, 2015
 *      Author: abhijeet
 */

#include <logger.h>
#include <ctype.h>
#include <stdarg.h>
#include <string.h>

FILE *fp = NULL;

void log_to_file(char *msg, ...)
{
	va_list	arglist;
	size_t size = 100;
	char buffer[100];

	if(fp == NULL)
	{
		fp = fopen(LOG_FILENAME, "w");
	}
	fprintf(fp, "LOGGER:%d ",__LINE__);

	va_start(arglist,msg);

	vsnprintf(buffer, size, msg, arglist);

	va_end(arglist);

	fwrite(buffer, strlen(buffer), 1, fp);

	fflush(fp);
}



