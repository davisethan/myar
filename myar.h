#include <ctype.h>
#include <dirent.h>
#include <stdbool.h>
#include "deque.h"


int shouldAppend(char **argv){
    /**
     * Archive should append on-disk file(s)
     * :param argv: Command arguments
     * :return: Should append
     */
    char *option = argv[1];
    return strcmp(option, "-q") == 0;
}

void doAppend(int argc, char **argv){
    /**
     * Append on-disk file(s) to archive
     * :param argc: Command arguments count
     * :pararm argv: Command arguments
     * :return: None
     */
    // Read archive
    char *archive = argv[2];
    forceOpenCloseArchive(archive);
    dequeStruct *deque = archiveToDequeStruct(archive);

    // Append unarchived file(s)
    char *file;
    for(int i=3; i < argc; i++){
        file = argv[i];
        dequeStructAppendArchivedFile(deque, file);
    }

    // Overwrite archive
    dequeStructToArchive(deque, archive);
    dequeFree(deque);
}

int shouldExtract(char **argv){
    /**
     * Should extract archived files to on-disk
     * :param argv: Command arguments
     * :return: Should extract archived files to on-disk
     */
    char *option = argv[1];
    return strcmp(option, "-x") == 0;
}

void doExtract(int argc, char **argv){
    /**
     * Extract archived files to on-disk
     * :param argc: Command arguments count
     * :param argv: Command arguments
     * :return: None
     */
    char *archive = argv[2];
    dequeStruct *deque = archiveToDequeStruct(archive);
    if(argc > 3){ // Extract filtered archive
        char *file;
        for(int i=3; i < argc; i++){
            file = argv[i];
            dequeNodeStruct *cur = deque->front->next;
            while(cur->next != NULL){
                archivedFileStruct *archivedFile = cur->data;
                if(strncmp(archivedFile->header->ar_name, file, strlen(file)) == 0){
                    archivedFileStructToFile(archivedFile);
                }
                cur = cur->next;
            }
        }
    }else{ // Extract unfiltered archive
        dequeNodeStruct *cur = deque->front->next;
        while(cur->next != NULL){
            archivedFileStruct *archivedFile = cur->data;
            archivedFileStructToFile(archivedFile);
            cur = cur->next;
        }
    }
    dequeFree(deque);
}

int shouldPrintConciseTable(char **argv){
    /**
     * Should print concise table of archive
     * :param argv: Command arguments
     * :return: Should print concise table of archive
     */
    char *option = argv[1];
    return strcmp(option, "-t") == 0;
}

void doPrintConciseTable(int argc, char **argv){
    /**
     * Do print concise table of archive
     * :param argc: Command arguments count
     * :param argv: Command arguments
     * :return: None
     */
    char *archive = argv[2];
    dequeStruct *deque = archiveToDequeStruct(archive);
    if(argc > 3){ // Print filtered concise table
        char *file;
        for(int i=3; i < argc; i++){
            file = argv[i];
            dequePrintArName(deque, file);
        }
    }else{ // Print unfiltered concise table
        dequePrintArNames(deque);
    }
    dequeFree(deque);
}

int shouldPrintVerboseTable(char **argv){
    /**
     * Should print verbose table of archive
     * :param argv: Command arguments
     * :return: Should print verbose table of archive
     */
    char *option = argv[1];
    return strcmp(option, "-v") == 0;
}

void doPrintVerboseTable(int argc, char **argv){
    /**
     * Print verbose table of archive
     * :param argc: Command arguments count
     * :param argv: Command arguments
     * :return: None
     */
    char *archive = argv[2];
    dequeStruct *deque = archiveToDequeStruct(archive);
    if(argc > 3){ // Print verbose table filtered archive
        char *file;
        for(int i=3; i < argc; i++){
            file = argv[i];
            dequeNodeStruct *cur = deque->front->next;
            while(cur->next != NULL){
                archivedFileStruct *archivedFile = cur->data;
                if(strncmp(archivedFile->header->ar_name, file, strlen(file)) == 0){
                    archivedFileStructPrintVerbose(archivedFile);
                }
                cur = cur->next;
            }
        }
    }else{ // Print verbose table unfiltered archive
        dequeNodeStruct *cur = deque->front->next;
        while(cur->next != NULL){
            archivedFileStruct *archivedFile = cur->data;
            archivedFileStructPrintVerbose(archivedFile);
            cur = cur->next;
        }
    }
    dequeFree(deque);
}

int shouldDelete(char **argv){
    /**
     * Should delete file(s) from archive
     * :param argv: Command arguments
     * :return: Should delete file(s) from archive
     */
    char *option = argv[1];
    return strcmp(option, "-d") == 0;
}

void doDelete(int argc, char **argv){
    /**
     * Delete file(s) from archive
     * :param argc: Command arguments count
     * :param argv: Command arguments
     * :return: Delete file(s) from archive
     */
    // Read archive
    char *archive = argv[2];
    dequeStruct *deque = archiveToDequeStruct(archive);

    // Delete archived file(s)
    char *file;
    for(int i=3; i < argc; i++){
        file = argv[i];
        dequeStructDeleteArchivedFile(deque, file);
    }

    // Overwrite archive
    dequeStructToArchive(deque, archive);
    dequeFree(deque);
}

int shouldAppendAll(char **argv){
    /**
     * Should append all regular files in current directory
     * :param argv: Command arguments
     * :return: Should append all regular files in current directory
     */
    char *option = argv[1];
    return strcmp(option, "-A") == 0;
}

void doAppendAll(int argc, char **argv){
    /**
     * Append all regular files in current directory
     * :param argc: Command arguments count
     * :param argv: Command arguments
     * :return: None
     */
    // Read archive
    char *archive = argv[2];
    dequeStruct *deque = archiveToDequeStruct(archive);

    // Read current directory
    DIR *curdir = opendir(".");
    struct dirent *file;
    while((file = readdir(curdir)) != NULL){
        // Current file is a text file
        char *buffer;
        struct stat filedata;
        stat(file->d_name, &filedata);
        int fd = open(file->d_name, O_RDONLY, 0666);
        lseek(fd, 0, SEEK_SET);
        buffer = malloc(filedata.st_size*sizeof(char));
        memset(buffer, '\0', filedata.st_size*sizeof(char));
        read(fd, buffer, filedata.st_size*sizeof(char));
        bool isTextFile = true;
        for(int i=0; i < filedata.st_size; i++){
            if(!isalnum(buffer[i]) && !isspace(buffer[i]) && !ispunct(buffer[i])){
                isTextFile = false;
            }
        }
        free(buffer);

        // Current file is an (different) archive file
        lseek(fd, 0, SEEK_SET);
        buffer = malloc(SARMAG*sizeof(char));
        memset(buffer, '\0', SARMAG*sizeof(char));
        read(fd, buffer, SARMAG*sizeof(char));
        bool isArchiveFile = strncmp(buffer, ARMAG, strlen(ARMAG)) == 0;
        bool isDiffArchiveFile = isArchiveFile && strncmp(archive, file->d_name, strlen(file->d_name)) != 0;
        close(fd);
        free(buffer);

        // Archive structured data deque append on-disk file
        if((isTextFile && !isArchiveFile) || isDiffArchiveFile){
            dequeStructAppendArchivedFile(deque, file->d_name);
        }
    }

    // Overwrite archive
    dequeStructToArchive(deque, archive);
    dequeFree(deque);
}
