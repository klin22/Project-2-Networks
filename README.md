# Project 2 - CNT4007 - UFmyMusic

This project implements a file synchronization system between a client and a server. The client can list files on the server, compare files between the client and server, and pull missing or different files from the server.

## Build Instructions

To build the project, use the provided Makefile. Run the following command in the terminal:

### Using WSL or Linux

```bash
make
```

### Using MacOS

Make sure to use brew because the make file points to a specific brew directory. If this does not find openssl, please provide the location of you're openssl installation

```zsh
brew install openssl
make
```

### Custom OpenSSL location

Replace the path with you're openssl path

```zsh
make CFLAGS="-I/path/to/openssl/include" LDFLAGS="-L/path/to/openssl/lib"
```

Following the steps above will compile both the client and server programs.

## Running the Server

To run the server, execute the following command:

```zsh
./server
```

The server will start listening on port 8080 and will serve files from the data directory.

## Running the Client

To run the client, execute the following command in a **different terminal**:

```zsh
./client
```

The client will connect to the server on port 8080 and will operate from the data directory.

## Client Operations

The client can perform the following operations:

- `1. List Files`: List all files available on the server.
- `2. Diff`: Compare files between the client and server and list files that are different or missing on the client.
- `3. Pull`: Pull missing or different files from the server to the client.
- `4. Leave`: End the session and close the connection.

## Dependencies

- `OpenSSL`: Used for computing MD5 hashes.
- `pthread`: Used for handling multiple client connections on the server.

## Authors

Kenny Lin & Nathan Wand
