#include <stdio.h>
#include "ar.h"


void forceOpenCloseArchive(char *pathname);
int openFileReadOnly(char *pathname);
int openFileWriteOnlyTruncate(char *pathname);
int openFileWriteOnlyCreateTruncate(char *pathname);


void forceOpenCloseArchive(char *pathname){
    /**
     * Confirm archive exists
     * :param pathname: Archive path
     * :return: None
     */
    int fd = open(pathname, O_RDONLY, 0666);
    if(fd >= 0){ // archive exists
        char *buffer = malloc(SARMAG);
        lseek(fd, 0, SEEK_SET);
        int bytesRead = read(fd, buffer, SARMAG);
        if(bytesRead == -1){
            fprintf(stderr, "Error: Cannot read from file \"%s\"\n", pathname);
            exit(EXIT_FAILURE);
        }else if(strcmp(buffer, ARMAG) != 0){
            fprintf(stderr, "Error: Non-archive file \"%s\"\n", pathname);
            exit(EXIT_FAILURE);
        }
        free(buffer);
    }else{
        fd = open(pathname, O_WRONLY | O_CREAT, 0666);
        if(fd == -1){
            fprintf(stderr, "Error: Cannot create file \"%s\"\n", pathname);
            exit(EXIT_FAILURE);
        }
        int bytesWritten = write(fd, ARMAG, SARMAG);
        if(bytesWritten == -1){
            fprintf(stderr, "Error: Cannot write to file \"%s\"\n", pathname);
            exit(EXIT_FAILURE);
        }
    }
    close(fd);
}

int openFileReadOnly(char *pathname){
    /**
     * Read only open file descriptor
     * :param pathname: File path
     * :return: Read only open file descriptor
     */
    int fd = open(pathname, O_RDONLY, 0666);
    if(fd == -1){
        fprintf(stderr, "Error: Cannot read only open file \"%s\"\n", pathname);
        exit(EXIT_FAILURE);
    }
    return fd;
}

int openFileWriteOnlyTruncate(char *pathname){
    /**
     * Write only open file descriptor
     * :param pathname: File path
     * :return: Write only open file descriptor
     */
    int fd = open(pathname, O_WRONLY | O_TRUNC, 0666);
    if(fd == -1){
        fprintf(stderr, "Error: Cannot write only truncate open file \"%s\"\n", pathname);
        exit(EXIT_FAILURE);
    }
    return fd;
}

int openFileWriteOnlyCreateTruncate(char *pathname){
    /**
     * Write only open file descriptor
     * :param pathname: File path
     * :return: Write only open file descriptor
     */
    int fd = open(pathname, O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if(fd == -1){
        fprintf(stderr, "Error: Cannot write only create truncate open file \"%s\"\n", pathname);
        exit(EXIT_FAILURE);
    }
    return fd;
}
