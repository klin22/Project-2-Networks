#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <dirent.h>
#include <stdlib.h>
#include <stdint.h> //for uint64
#include "ufmymusic.h"  // Include the header file

#include <openssl/md5.h> //for hashing diff

#define PORT 8080

void *client_handler(void *socket_desc);
void list_files(int client_sock);
void diff_files(int client_sock);
void pull_files(int client_sock);
FileList get_server_files();
int compute_file_md5(const char *filename, unsigned char *md5_digest);
ssize_t send_all(int sockfd, const void *buf, size_t len);
ssize_t recv_all(int sockfd, void *buf, size_t len);

int main() {
    if (chdir("./serverdir/data") != 0) {
        perror("Failed to change directory to ./server/data");
        exit(EXIT_FAILURE);
    }
    int server_fd, client_sock, c;
    struct sockaddr_in server, client;
    pthread_t thread_id;

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if(server_fd < 0){
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr *)&server, sizeof(server)) < 0) {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }
    if (listen(server_fd, 3) < 0) {
        perror("Listen failed");
        exit(EXIT_FAILURE);
}

    printf("Server listening on port %d...\n", PORT);
    c = sizeof(struct sockaddr_in);
    while (1) {
        client_sock = accept(server_fd, (struct sockaddr *)&client, (socklen_t *)&c);
        if (client_sock < 0) {
            perror("Accept failed");
            continue; // Optionally, you may choose to exit or handle differently
        }
        // Rest of your code
        int* new_sock = malloc(sizeof(int));
        if(new_sock == NULL){
            perror("Failed to allocate memory");
            close(client_sock); // Close the client socket since we can't handle it
            continue;
        }
        *new_sock = client_sock;
        if (pthread_create(&thread_id, NULL, client_handler, (void *)new_sock) != 0) {
            perror("Thread creation failed");
            close(client_sock);
            free(new_sock);
            continue;
        }
        pthread_detach(thread_id);
    }

    close(server_fd);
    return 0;
}

void *client_handler(void *socket_desc) {
    int client_sock = *(int *)socket_desc;
    free(socket_desc);
    Message msg;
    ssize_t bytes_received;
    while (1) {
        bytes_received = recv_all(client_sock, &msg, sizeof(msg));
        if (bytes_received <= 0) {
            if (bytes_received < 0) {
                perror("Receive failed");
            } else {
                printf("Client disconnected.\n");
            }
            close(client_sock);
            pthread_exit(NULL);
        }

        switch (msg.type) {
            case 1:
                list_files(client_sock);
                break;
            case 2:
                diff_files(client_sock);
                break;
            case 3:
                pull_files(client_sock);
                break;
            case 4:
                printf("Client requested to leave.\n");
                close(client_sock);
                pthread_exit(NULL);
            default:
                printf("Invalid request.\n");
        }
    }
}

int compute_file_md5(const char *filename, unsigned char *md5_digest) {
    FILE *file = fopen(filename, "rb");
    if (!file) {
        perror("Cannot open file to compute MD5");
        return -1;
    }

    MD5_CTX mdContext;
    unsigned char data[1024];
    size_t bytes;

    MD5_Init(&mdContext);
    while ((bytes = fread(data, 1, sizeof(data), file)) != 0) {
        MD5_Update(&mdContext, data, bytes);
    }
    MD5_Final(md5_digest, &mdContext);
    fclose(file);
    return 0;
}

// Function to list files in the server's directory and return them
FileList get_server_files() {
    DIR *d;
    struct dirent *dir;
    FileList filelist;
    filelist.file_count = 0;

    d = opendir(".");
    if (d) {
        while ((dir = readdir(d)) != NULL) {
            if (dir->d_type == DT_REG) {
                if (filelist.file_count >= MAX_FILES) {
                    break;
                }
                strncpy(filelist.files[filelist.file_count].filename, dir->d_name, MAX_FILENAME_LEN - 1);
                filelist.files[filelist.file_count].filename[MAX_FILENAME_LEN - 1] = '\0';
                if (compute_file_md5(dir->d_name, filelist.files[filelist.file_count].md5_hash) != 0) {
                    fprintf(stderr, "Failed to compute MD5 for file: %s\n", dir->d_name);
                    continue;
                }
                filelist.file_count++;
            }
        }
        closedir(d);
    }
    else {
        perror("opendir");  // Print error if directory can't be opened
    }
    return filelist;
}
ssize_t send_all(int sockfd, const void *buf, size_t len) {
    size_t total_sent = 0;
    const char *buffer = buf;
    while (total_sent < len) {
        ssize_t bytes_sent = send(sockfd, buffer + total_sent, len - total_sent, 0);
        if (bytes_sent <= 0) {
            perror("Send failed");
            return -1;
        }
        total_sent += bytes_sent;
    }
    return total_sent;
}
ssize_t recv_all(int sockfd, void *buf, size_t len) {
    size_t total_received = 0;
    char *buffer = buf;
    while (total_received < len) {
        ssize_t bytes_received = recv(sockfd, buffer + total_received, len - total_received, 0);
        if (bytes_received < 0) {
            perror("Receive failed");
            return -1;
        } else if (bytes_received == 0) {
            // Connection closed by the client
            break;
        }
        total_received += bytes_received;
    }
    return total_received;
}
// Function to send the server's file list to the client
void list_files(int client_sock) {
    printf("Handling LIST request for client %d\n", client_sock);
    FileList filelist = get_server_files();  // Use helper to get server files
    ssize_t bytes_sent = send_all(client_sock, &filelist, sizeof(filelist));
    if (bytes_sent != sizeof(filelist)) {
        perror("Send failed");
    }
}

// Function to compare files between client and server (Diff functionality)
void diff_files(int client_sock) {
    printf("Handling DIFF request for client %d\n", client_sock);
    FileList client_files, server_files, diff;
    diff.file_count = 0;

    // Receive client file list
    if (recv_all(client_sock, &client_files, sizeof(client_files)) <= 0) {
        perror("Receive client file list failed");
        close(client_sock);
        return;
    }

    // Get the list of files on the server using the helper function
    server_files = get_server_files();

    // Compare client and server files
    for (int i = 0; i < server_files.file_count; i++) {
        int found = 0;
        for (int j = 0; j < client_files.file_count; j++) {
            if (memcmp(server_files.files[i].md5_hash, client_files.files[j].md5_hash, MD5_DIGEST_LENGTH) == 0) {
                found = 1;
                break;
            }
        }
        if (!found) {
            // If the file is on the server but not on the client, add it to the diff
            diff.files[diff.file_count++] = server_files.files[i];
        }
    }

    // Send the diff (missing files on the client) back to the client
    if (send_all(client_sock, &diff, sizeof(diff)) != sizeof(diff)) {
        perror("Send diff failed");
        close(client_sock);
        return;
    }
}

void pull_files(int client_sock) {
    printf("Handling PULL request for client %d\n", client_sock);
    // Receive client file list
    FileList client_files;
    if (recv_all(client_sock, &client_files, sizeof(client_files)) <= 0) {
        perror("Receive client file list failed");
        close(client_sock);
        return;
    }

    // Compute diff_files
    FileList server_files = get_server_files();
    FileList diff_files;
    diff_files.file_count = 0;
    for (int i = 0; i < server_files.file_count; i++) {
        int found = 0;
        for (int j = 0; j < client_files.file_count; j++) {
            if (memcmp(server_files.files[i].md5_hash, client_files.files[j].md5_hash, MD5_DIGEST_LENGTH) == 0) {
                found = 1;
                break;
            }
        }
        if (!found) {
            diff_files.files[diff_files.file_count++] = server_files.files[i];
        }
    }

    // Send diff_files to client
    if (send_all(client_sock, &diff_files, sizeof(diff_files)) != sizeof(diff_files)) {
        perror("Send diff_files failed");
        close(client_sock);
        return;
    }
    // if (recv_all(client_sock, &diff_files, sizeof(diff_files)) <= 0) {
    //     perror("Receive diff files failed");
    //     close(client_sock);
    //     return;
    // }  // Receive list of files to pull (from Diff)
    int num_files_to_send = diff_files.file_count;
    if (send_all(client_sock, &num_files_to_send, sizeof(num_files_to_send)) != sizeof(num_files_to_send)) {
        perror("Send number of files failed");
        close(client_sock);
        return;
    }
    // Iterate over the list of files the client wants to pull
    for (int i = 0; i < diff_files.file_count; i++) {
        char *filename = diff_files.files[i].filename;
        FILE *file = fopen(filename, "rb");

        if (file == NULL) {
            perror("Error opening file");
            continue;
        }

        // Send file name to the client
        size_t filename_len = strlen(filename) + 1;
        if (send_all(client_sock, &filename_len, sizeof(filename_len)) < 0 ||
            send_all(client_sock, filename, filename_len) < 0) {
            perror("Send filename failed");
            fclose(file);
            continue;
        }
        // Send the file size
        if (fseek(file, 0, SEEK_END) != 0) {
            perror("fseek failed");
            fclose(file);
            continue;
        }
        uint64_t file_size = ftell(file);
        if (file_size == (uint64_t)-1) {
            perror("ftell failed");
            fclose(file);
            continue;
        }
        rewind(file);
        if (send_all(client_sock, &file_size, sizeof(file_size)) < 0) {
            perror("Send file size failed");
            fclose(file);
            continue;
        }

        // Send file contents in chunks
        char buffer[1024];
        size_t bytes_read;
        while ((bytes_read = fread(buffer, 1, sizeof(buffer), file)) > 0) {
            if (send_all(client_sock, buffer, bytes_read) < 0) {
                perror("Send file content failed");
                break;
            }
        }

        fclose(file);  // Close the file after sending
    }
}
