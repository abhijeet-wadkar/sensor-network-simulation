CC = gcc
CCFLAGS = -DDEBUG -g -Wall
EXE_NAME = sensor
EXT_LIB = -lpthread

INCLUDES = src/common \
	src/sensor

SRCS = src/sensor/sensor.c \
	src/sensor/main.c \
	src/common/network_functions.c \
	src/common/network_read_thread.c \
	src/common/string_helper_functions.c
	
include base_make.mk

#all: ../src/sensor/sensor.c ../src/sensor/sensor.h ../src/common/string_helper_functions.c
#		$(CC) $(CCFLAGS) -o ../src/sensor/sensor.o ../src/common/string_helper_functions.c ../src/sensor/sensor.c -I../src/sensor -I../src/common