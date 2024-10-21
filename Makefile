# Makefile for compiling client and server from root directory

# Compiler and flags
CC = gcc
CFLAGS = -Wall -g

# Libraries
LIBS = -lssl -lcrypto
PTHREAD_LIBS = -lpthread

# Targets
CLIENT_TARGET = client
SERVER_TARGET = server

# Source files
CLIENT_SRCS = client.c
SERVER_SRCS = server.c

# Include directories
INCLUDES = -I.

# Phony targets
.PHONY: all client server clean

# Default target
all: client server

# Build client
client: $(CLIENT_TARGET)

$(CLIENT_TARGET): $(CLIENT_SRCS) ufmymusic.h
	$(CC) $(CFLAGS) $(INCLUDES) -o $@ $(CLIENT_SRCS) $(LIBS)

# Build server
server: $(SERVER_TARGET)

$(SERVER_TARGET): $(SERVER_SRCS) ufmymusic.h
	$(CC) $(CFLAGS) $(INCLUDES) -o $@ $(SERVER_SRCS) $(LIBS) $(PTHREAD_LIBS)

# Clean up
clean:
	rm -f $(CLIENT_TARGET) $(SERVER_TARGET)
