CC = gcc
CFLAGS = -Wall -g

OPENSSL_INCLUDE = -I/usr/include/openssl
OPENSSL_LIB = -L/usr/lib/openssl

UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S), Darwin)
    # macOS (Homebrew OpenSSL)
    OPENSSL_PATH = /opt/homebrew/opt/openssl@3
    OPENSSL_INCLUDE = -I$(OPENSSL_PATH)/include
    OPENSSL_LIB = -L$(OPENSSL_PATH)/lib
endif

LIBS = -lssl -lcrypto
PTHREAD_LIBS = -lpthread

CLIENT_TARGET = client
SERVER_TARGET = server

CLIENT_SRCS = client.c
SERVER_SRCS = server.c

INCLUDES = -I. $(OPENSSL_INCLUDE)

.PHONY: all client server clean

all: client server

client: $(CLIENT_TARGET)

$(CLIENT_TARGET): $(CLIENT_SRCS) ufmymusic.h
	$(CC) $(CFLAGS) $(INCLUDES) $(OPENSSL_LIB) -o $@ $(CLIENT_SRCS) $(LIBS)

server: $(SERVER_TARGET)

$(SERVER_TARGET): $(SERVER_SRCS) ufmymusic.h
	$(CC) $(CFLAGS) $(INCLUDES) $(OPENSSL_LIB) -o $@ $(SERVER_SRCS) $(LIBS) $(PTHREAD_LIBS)

clean:
	rm -f $(CLIENT_TARGET) $(SERVER_TARGET)
