CC = gcc
CCFLAGS = -DDEBUG -g -Wall
EXE_NAME = gateway
EXT_LIB = -lpthread

INCLUDES = src/common \
	src/gateway

SRCS = src/gateway/gateway.c \
	src/gateway/gateway_main.c \
	src/common/network_functions.c \
	src/common/network_read_thread.c \
	src/common/string_helper_functions.c
	
include base_make.mk