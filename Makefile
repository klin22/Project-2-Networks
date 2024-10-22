# Compiler and flags
CC = gcc
CFLAGS = -Wall -g

# Default OpenSSL paths (will be set per platform)
OPENSSL_INCLUDE = 
OPENSSL_LIB = 

# Detect platform and set OpenSSL paths accordingly
UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S), Darwin)
    # macOS (Homebrew OpenSSL)
    OPENSSL_PATH = /opt/homebrew/opt/openssl@3
    OPENSSL_INCLUDE = -I$(OPENSSL_PATH)/include
    OPENSSL_LIB = -L$(OPENSSL_PATH)/lib
else ifeq ($(UNAME_S), Linux)
    # Linux/WSL paths (assuming OpenSSL installed via package manager)
    OPENSSL_INCLUDE = -I/usr/include/openssl
    OPENSSL_LIB = -L/usr/lib/openssl
endif

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
INCLUDES = -I. $(OPENSSL_INCLUDE)

# Phony targets
.PHONY: all client server clean

# Default target
all: client server

# Build client
client: $(CLIENT_TARGET)

$(CLIENT_TARGET): $(CLIENT_SRCS) ufmymusic.h
	$(CC) $(CFLAGS) $(INCLUDES) $(OPENSSL_LIB) -o $@ $(CLIENT_SRCS) $(LIBS)

# Build server
server: $(SERVER_TARGET)

$(SERVER_TARGET): $(SERVER_SRCS) ufmymusic.h
	$(CC) $(CFLAGS) $(INCLUDES) $(OPENSSL_LIB) -o $@ $(SERVER_SRCS) $(LIBS) $(PTHREAD_LIBS)

# Clean up
clean:
	rm -f $(CLIENT_TARGET) $(SERVER_TARGET)
