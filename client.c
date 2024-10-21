#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <dirent.h>
#include <stdint.h>
#include <openssl/md5.h>
#include "ufmymusic.h"
#define PORT 8080

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
void get_client_files(FileList *filelist) {
    DIR *d;
    struct dirent *dir;
    filelist->file_count = 0;

    d = opendir(".");
    if (d) {
        while ((dir = readdir(d)) != NULL) {
            if (dir->d_type == DT_REG) {
                if (filelist->file_count >= MAX_FILES) {
                    break;
                }
                strncpy(filelist->files[filelist->file_count].filename, dir->d_name, MAX_FILENAME_LEN - 1);
                filelist->files[filelist->file_count].filename[MAX_FILENAME_LEN - 1] = '\0';
                if (compute_file_md5(dir->d_name, filelist->files[filelist->file_count].md5_hash) != 0) {
                    fprintf(stderr, "Failed to compute MD5 for file: %s\n", dir->d_name);
                    continue;
                }
                filelist->file_count++;
            }
        }
        closedir(d);
    }
    else {
        perror("opendir");
    }
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
            // Connection closed by the server
            break;
        }
        total_received += bytes_received;
    }
    return total_received;
}

int main() {
    int sock;
    struct sockaddr_in server;
    Message msg;
    FileList filelist, server_files;

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("Socket creation failed");
        return 1;
    }
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons(PORT);

    if (connect(sock, (struct sockaddr *)&server, sizeof(server)) < 0) {
        perror("Connection failed");
        return 1;
    }

    int choice;
    printf("1. List Files\n2. Diff\n3. Pull\n4. Leave\nChoose an action: ");
    if (scanf("%d", &choice) != 1) {
        fprintf(stderr, "Invalid input\n");
        close(sock);
        return 1;
    }
    if (choice < 1 || choice > 4) {
        fprintf(stderr, "Invalid choice\n");
        close(sock);
        return 1;
    }
    msg.type = choice;

    send(sock, &msg, sizeof(msg), 0);
    //LIST  
    if (choice == 1) {
        if (recv_all(sock, &server_files, sizeof(server_files)) != sizeof(server_files)) {
            perror("Receive failed");
            close(sock);
            return 1;
        }
        printf("Server has the following files:\n");
        for (int i = 0; i < server_files.file_count; i++) {
            printf("%s\n", server_files.files[i].filename);
        }
    }
    //DIFF OR PULL
    else if (choice == 2 || choice == 3) {
        // Diff or Pull - Send client file list to server
        get_client_files(&filelist);
        if (send_all(sock, &filelist, sizeof(filelist)) != sizeof(filelist)) {
            perror("Send failed");
            close(sock);
            return 1;
        }

        if (choice == 2) {
            // Receive file differences for Diff
            if (recv_all(sock, &server_files, sizeof(server_files)) != sizeof(server_files)) {
                perror("Receive failed");
                close(sock);
                return 1;
            }
            printf("Files on server that are different from client:\n");
            for (int i = 0; i < server_files.file_count; i++) {
                printf("%s\n", server_files.files[i].filename);
            }
            } else if (choice == 3) {
                    // First, get client file list and send to server
            get_client_files(&filelist);
            if (send_all(sock, &filelist, sizeof(filelist)) != sizeof(filelist)) {
                perror("Send failed");
                close(sock);
                return 1;
            }

            // Receive diff from server
            FileList diff_files;
            if (recv_all(sock, &diff_files, sizeof(diff_files)) != sizeof(diff_files)) {
                perror("Receive diff failed");
                close(sock);
                return 1;
            }

            // Send back the diff_files to request files to pull
            if (send_all(sock, &diff_files, sizeof(diff_files)) != sizeof(diff_files)) {
                perror("Send diff_files failed");
                close(sock);
                return 1;
            }
            // Receive the number of files the server will send
            int num_files_to_receive;
            if (recv_all(sock, &num_files_to_receive, sizeof(num_files_to_receive)) != sizeof(num_files_to_receive)) {
                perror("Receive number of files failed");
                close(sock);
                return 1;
            }

            // Receive files from server
            for (int i = 0; i < num_files_to_receive; i++) {
                // Receive filename length
                size_t filename_len;
                if (recv_all(sock, &filename_len, sizeof(filename_len)) != sizeof(filename_len)) {
                    perror("Receive filename length failed");
                    close(sock);
                    return 1;
                }

                // Receive filename
                char filename[MAX_FILENAME_LEN];
                if (recv_all(sock, filename, filename_len) != filename_len) {
                    perror("Receive filename failed");
                    close(sock);
                    return 1;
                }

                // Receive file size
                uint64_t file_size;
                if (recv_all(sock, &file_size, sizeof(file_size)) != sizeof(file_size)) {
                    perror("Receive file size failed");
                    close(sock);
                    return 1;
                }

                // Receive file content
                FILE *file = fopen(filename, "wb");
                if (!file) {
                    perror("Cannot open file to write");
                    // Optionally, consume the data from the socket to keep it in sync
                    uint64_t bytes_to_discard = file_size;
                    char discard_buffer[1024];
                    while (bytes_to_discard > 0) {
                        size_t to_receive = (bytes_to_discard < sizeof(discard_buffer)) ? bytes_to_discard : sizeof(discard_buffer);
                        ssize_t bytes_received = recv_all(sock, discard_buffer, to_receive);
                        if (bytes_received <= 0) {
                            perror("Receive file content failed");
                            close(sock);
                            return 1;
                        }
                        bytes_to_discard -= bytes_received;
                    }
                    continue;
                }

                uint64_t total_received = 0;
                char buffer[1024];
                while (total_received < file_size) {
                    size_t to_receive = (file_size - total_received < sizeof(buffer)) ? (file_size - total_received) : sizeof(buffer);
                    ssize_t bytes_received = recv_all(sock, buffer, to_receive);
                    if (bytes_received <= 0) {
                        perror("Receive file content failed");
                        fclose(file);
                        close(sock);
                        return 1;
                    }
                    fwrite(buffer, 1, bytes_received, file);
                    total_received += bytes_received;
                }
                fclose(file);
                printf("Received file: %s\n", filename);
            }
        }
    }
    //LEAVE
    else if (choice == 4) {
        printf("Ending session and closing connection.\n");
        close(sock);
        return 0;
    }

    close(sock);
    return 0;
}
