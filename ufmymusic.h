#ifndef UFMYMUSIC_H
#define UFMYMUSIC_H

#define MAX_FILENAME_LEN 256
#define MAX_FILES 100

#define MD5_DIGEST_LENGTH 16

typedef struct {
    int type;  
} Message;

typedef struct {
    char filename[MAX_FILENAME_LEN];
    unsigned char md5_hash[MD5_DIGEST_LENGTH];
} FileEntry;

typedef struct {
    FileEntry files[MAX_FILES];
    int file_count;
} FileList;

#endif
