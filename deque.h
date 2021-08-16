#include <fcntl.h>
#include <libgen.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>
#include <utime.h>
#include "file.h"


#define AR_NAME_SIZE 16
#define AR_DATE_SIZE 12
#define AR_UID_SIZE 6
#define AR_GID_SIZE 6
#define AR_MODE_SIZE 8
#define AR_SIZE_SIZE 10
#define AR_FMAG_SIZE 2


typedef struct ar_hdr archivedFileHeaderStruct;

typedef struct archivedFile{
    archivedFileHeaderStruct *header;
    char *body;
}archivedFileStruct;

typedef struct dequeNode{
    archivedFileStruct *data;
    struct dequeNode *next;
    struct dequeNode *prev;
}dequeNodeStruct;

typedef struct deque{
    dequeNodeStruct *front;
    dequeNodeStruct *rear;
}dequeStruct;


void dequeAppendRear(dequeStruct *deque, archivedFileStruct *archivedFile);
void dequePrint(dequeStruct *deque);
void dequePrintArNames(dequeStruct *deque);
void dequePrintArName(dequeStruct *deque, char *filename);
void dequeFree(dequeStruct *deque);
void dequeNodeFree(dequeNodeStruct *dequeNode);
dequeStruct *archiveToDequeStruct(char *pathname);
archivedFileStruct *archivedFileToArchivedFileStruct(int fd);
void dequeStructToArchive(dequeStruct *deque, char *pathname);
void archivedFileStructToFile(archivedFileStruct *archivedFile);
void archivedFileStructPrintVerbose(archivedFileStruct *archivedFile);
char *monthName(int month);
void dequeStructAppendArchivedFile(dequeStruct *deque, char *pathname);
void dequeStructDeleteArchivedFile(dequeStruct *deque, char *pathname);


void dequeAppendRear(dequeStruct *deque, archivedFileStruct *archivedFile){
    /**
     * Deque append rear node
     * :param deque: Deque data structure
     * :param archivedFile: Deque node data
     * :return: None
     */
    dequeNodeStruct *dequeNode = malloc(sizeof(dequeNodeStruct));
    dequeNode->data = archivedFile;
    dequeNode->next = deque->rear;
    dequeNode->prev = deque->rear->prev;
    deque->rear->prev->next = dequeNode;
    deque->rear->prev = dequeNode;
}

void dequePrint(dequeStruct *deque){
    /**
     * Print deque data structure to CLI
     * :param deque: Deque data structure
     * :return: None
     */
    dequeNodeStruct *cur = deque->front->next;
    while(cur->next != NULL){
        printf("\"%s\"\n", cur->data->header->ar_name);
        printf("\"%s\"\n", cur->data->header->ar_date);
        printf("\"%s\"\n", cur->data->header->ar_uid);
        printf("\"%s\"\n", cur->data->header->ar_gid);
        printf("\"%s\"\n", cur->data->header->ar_mode);
        printf("\"%s\"\n", cur->data->header->ar_size);
        printf("\"%s\"\n", cur->data->header->ar_fmag);
        printf("\"%s\"\n", cur->data->body);
        cur = cur->next;
    }
}

void dequePrintArNames(dequeStruct *deque){
    /**
     * Print deque data structure ar_names to terminal
     * :param deque: Archive structured data deque
     * :return: None
     */
    dequeNodeStruct *cur = deque->front->next;
    while(cur->next != NULL){
        printf("%s\n", cur->data->header->ar_name);
        cur = cur->next;
    }
}

void dequePrintArName(dequeStruct *deque, char *filename){
    /**
     * Print deque data structure matching filename ar_name to terminal
     * :param deque: Archived structured data deque
     * :param filename: Filename ar_name to match
     * :return: None
     */
    dequeNodeStruct *cur = deque->front->next;
    while(cur->next != NULL){
        if(strncmp(cur->data->header->ar_name, filename, strlen(filename)) == 0){
            printf("%s\n", cur->data->header->ar_name);
            break;
        }
        cur = cur->next;
    }
}

void dequeFree(dequeStruct *deque){
    /**
     * Free deque data structure heap memory
     * :param deque: Deque data structure
     * :return: None
     */
    dequeNodeStruct *cur = deque->front;
    dequeNodeStruct *next = cur->next;
    while(cur != NULL){
        if(cur->data != NULL){
            archivedFileStruct *archivedFile = cur->data;
            free(archivedFile->header);
            free(archivedFile->body);
            free(archivedFile);
        }
        free(cur);
        cur = next;
        if(cur != NULL){
            next = cur->next;
        }
    }
    free(deque);
}

void dequeNodeFree(dequeNodeStruct *dequeNode){
    /**
     * Free deque node data structure heap memory
     * :param dequeNode: Deque node data structure
     * :return: None
     */
    free(dequeNode->data->header);
    free(dequeNode->data->body);
    free(dequeNode->data);
    free(dequeNode);
}

dequeStruct *archiveToDequeStruct(char *pathname){
    /**
     * Read on-disk file to structured data deque
     * :param pathname: On-disk file path
     * :return: Deque data structure
     */
    int fd = openFileReadOnly(pathname);

    // Create deque
    dequeNodeStruct *front = malloc(sizeof(dequeNodeStruct));
    dequeNodeStruct *rear = malloc(sizeof(dequeNodeStruct));
    front->data = NULL;
    front->next = rear;
    front->prev = NULL;
    rear->data = NULL;
    rear->next = NULL;
    rear->prev = front;
    dequeStruct *deque = malloc(sizeof(dequeStruct));
    deque->front = front;
    deque->rear = rear;

    // Fill deque
    int endOffset = lseek(fd, 0, SEEK_END);
    lseek(fd, SARMAG, SEEK_SET);
    int curOffset = lseek(fd, 0, SEEK_CUR);
    while(curOffset < endOffset-1){
        archivedFileStruct *archivedFile = archivedFileToArchivedFileStruct(fd);
        dequeAppendRear(deque, archivedFile);
        curOffset = lseek(fd, 0, SEEK_CUR);
    }

    close(fd);
    return deque;
}

archivedFileStruct *archivedFileToArchivedFileStruct(int fd){
    /**
     * Read on-disk archived file to structured data deque
     * Helper function to `archiveToDequeStruct`
     * :param fd: On-disk archived file open file descriptor
     * :return: Archived file structured data deque
     */
    // Create archivedFile
    archivedFileHeaderStruct *header = malloc(sizeof(archivedFileHeaderStruct));
    archivedFileStruct *archivedFile = malloc(sizeof(archivedFileStruct));
    archivedFile->header = header;
    archivedFile->body = NULL;

    // Fill archivedFile
    char *buffer;
    int bytesRead;

    // Read archived file name
    buffer = malloc(AR_NAME_SIZE);
    memset(buffer, '\0', AR_NAME_SIZE);
    bytesRead = read(fd, buffer, AR_NAME_SIZE);
    if(bytesRead != AR_NAME_SIZE){
        fprintf(stderr, "Error: Cannot read ar_name from archive\n");
        exit(EXIT_FAILURE);
    }
    buffer[AR_NAME_SIZE-1] = '\0';
    memcpy(header->ar_name, buffer, AR_NAME_SIZE);
    free(buffer);

    // Read archived file date
    buffer = malloc(AR_DATE_SIZE);
    memset(buffer, '\0', AR_DATE_SIZE);
    bytesRead = read(fd, buffer, AR_DATE_SIZE);
    if(bytesRead != AR_DATE_SIZE){
        fprintf(stderr, "Error: Cannot read ar_date from archive\n");
        exit(EXIT_FAILURE);
    }
    buffer[AR_DATE_SIZE-1] = '\0';
    memcpy(header->ar_date, buffer, AR_DATE_SIZE);
    free(buffer);

    // Read archived file uid
    buffer = malloc(AR_UID_SIZE);
    memset(buffer, '\0', AR_UID_SIZE);
    bytesRead = read(fd, buffer, AR_UID_SIZE);
    if(bytesRead != AR_UID_SIZE){
        fprintf(stderr, "Error: Cannot read ar_uid from archive\n");
        exit(EXIT_FAILURE);
    }
    buffer[AR_UID_SIZE-1] = '\0';
    memcpy(header->ar_uid, buffer, AR_UID_SIZE);
    free(buffer);

    // Read archived file gid
    buffer = malloc(AR_GID_SIZE);
    memset(buffer, '\0', AR_GID_SIZE);
    bytesRead = read(fd, buffer, AR_GID_SIZE);
    if(bytesRead != AR_GID_SIZE){
        fprintf(stderr, "Error: Cannot read ar_gid from archive\n");
        exit(EXIT_FAILURE);
    }
    buffer[AR_GID_SIZE-1] = '\0';
    memcpy(header->ar_gid, buffer, AR_GID_SIZE);
    free(buffer);

    // Read archived file mode
    buffer = malloc(AR_MODE_SIZE);
    memset(buffer, '\0', AR_MODE_SIZE);
    bytesRead = read(fd, buffer, AR_MODE_SIZE);
    if(bytesRead != AR_MODE_SIZE){
        fprintf(stderr, "Error: Cannot read ar_mode from archive\n");
        exit(EXIT_FAILURE);
    }
    buffer[AR_MODE_SIZE-1] = '\0';
    memcpy(header->ar_mode, buffer, AR_MODE_SIZE);
    free(buffer);

    // Read archived file size
    buffer = malloc(AR_SIZE_SIZE);
    memset(buffer, '\0', AR_SIZE_SIZE);
    bytesRead = read(fd, buffer, AR_SIZE_SIZE);
    if(bytesRead != AR_SIZE_SIZE){
        fprintf(stderr, "Error: Cannot read ar_size from archive\n");
        exit(EXIT_FAILURE);
    }
    buffer[AR_SIZE_SIZE-1] = '\0';
    memcpy(header->ar_size, buffer, AR_SIZE_SIZE);
    free(buffer);

    // Read archived file fmag
    buffer = malloc(AR_FMAG_SIZE);
    memset(buffer, '\0', AR_FMAG_SIZE);
    bytesRead = read(fd, buffer, AR_FMAG_SIZE);
    if(bytesRead != AR_FMAG_SIZE){
        fprintf(stderr, "Error: Cannot read ar_fmag from archive\n");
        exit(EXIT_FAILURE);
    }
    memcpy(header->ar_fmag, buffer, AR_FMAG_SIZE);
    free(buffer);

    // Read archived file body
    int ar_size;
    sscanf(header->ar_size, "%d", &ar_size);
    buffer = malloc(ar_size);
    bytesRead = read(fd, buffer, ar_size);
    if(bytesRead != ar_size){
        fprintf(stderr, "Error: Cannot read body from archive\n");
        exit(EXIT_FAILURE);
    }
    archivedFile->body = malloc(ar_size*sizeof(char));
    memcpy(archivedFile->body, buffer, ar_size);
    free(buffer);

    return archivedFile;
}

void dequeStructToArchive(dequeStruct *deque, char *pathname){
    /**
     * Write structured data deque to on-disk archive file
     * :param deque: Archived structured data deque
     * :param pathname: On-disk archive file path
     * :return: None
     */
    int fd = openFileWriteOnlyTruncate(pathname);
    int bytesWritten;

    // Write archive indicator to archive
    lseek(fd, 0, SEEK_SET);
    bytesWritten = write(fd, ARMAG, SARMAG);
    if(bytesWritten == -1){
        fprintf(stderr, "Error: Cannot write archive indicator to file \"%s\"\n", pathname);
        exit(EXIT_FAILURE);
    }

    // Write deque to archive
    dequeNodeStruct *cur = deque->front->next;
    while(cur->next != NULL){
        // Write ar_name to archive
        bytesWritten = write(fd, cur->data->header->ar_name, AR_NAME_SIZE);
        if(bytesWritten == -1){
            fprintf(stderr, "Error: Cannot write ar_name to file \"%s\"\n", pathname);
            exit(EXIT_FAILURE);
        }

        // Write ar_date to archive
        bytesWritten = write(fd, cur->data->header->ar_date, AR_DATE_SIZE);
        if(bytesWritten == -1){
            fprintf(stderr, "Error: Cannot write ar_date to file \"%s\"\n", pathname);
            exit(EXIT_FAILURE);
        }

        // Write ar_uid to archive
        bytesWritten = write(fd, cur->data->header->ar_uid, AR_UID_SIZE);
        if(bytesWritten == -1){
            fprintf(stderr, "Error: Cannot write ar_uid to file \"%s\"\n", pathname);
            exit(EXIT_FAILURE);
        }

        // Write ar_gid to archive
        bytesWritten = write(fd, cur->data->header->ar_gid, AR_GID_SIZE);
        if(bytesWritten == -1){
            fprintf(stderr, "Error: Cannot write ar_gid to file \"%s\"\n", pathname);
            exit(EXIT_FAILURE);
        }

        // Write ar_mode to archive
        bytesWritten = write(fd, cur->data->header->ar_mode, AR_MODE_SIZE);
        if(bytesWritten == -1){
            fprintf(stderr, "Error: Cannot write ar_mode to file \"%s\"\n", pathname);
            exit(EXIT_FAILURE);
        }

        // Write ar_size to archive
        bytesWritten = write(fd, cur->data->header->ar_size, AR_SIZE_SIZE);
        if(bytesWritten == -1){
            fprintf(stderr, "Error: Cannot write ar_size to file \"%s\"\n", pathname);
            exit(EXIT_FAILURE);
        }

        // Write ar_fmag to archive
        bytesWritten = write(fd, cur->data->header->ar_fmag, AR_FMAG_SIZE);
        if(bytesWritten == -1){
            fprintf(stderr, "Error: Cannot write ar_fmag to file \"%s\"\n", pathname);
            exit(EXIT_FAILURE);
        }

        // Write archived file data to archive
        int ar_size;
        sscanf(cur->data->header->ar_size, "%d", &ar_size);
        bytesWritten = write(fd, cur->data->body, ar_size);
        if(bytesWritten == -1){
            fprintf(stderr, "Error: Cannot write body to file \"%s\"\n", pathname);
            exit(EXIT_FAILURE);
        }

        cur = cur->next;
    }

    close(fd);
}

void archivedFileStructToFile(archivedFileStruct *archivedFile){
    // Write file body
    char *ar_name = archivedFile->header->ar_name;
    int fd = openFileWriteOnlyCreateTruncate(ar_name);
    int ar_size;
    sscanf(archivedFile->header->ar_size, "%d", &ar_size);
    int bytesWritten = write(fd, archivedFile->body, ar_size);
    if(bytesWritten != ar_size){
        fprintf(stderr, "Error: Cannot write body to file \"%s\"\n", ar_name);
        exit(EXIT_FAILURE);
    }

    // Change file permissions
    int ar_mode;
    sscanf(archivedFile->header->ar_mode, "%d", &ar_mode);
    int mod = chmod(ar_name, ar_mode);
    if(mod == -1){
        fprintf(stderr, "Error: Cannot change permissions on file \"%s\"\n", ar_name);
        exit(EXIT_FAILURE);
    }

    // Change file ownership
    int ar_uid;
    int ar_gid;
    sscanf(archivedFile->header->ar_uid, "%d", &ar_uid);
    sscanf(archivedFile->header->ar_gid, "%d", &ar_gid);
    int own = chown(ar_name, ar_uid, ar_gid);
    if(own == -1){
        fprintf(stderr, "Error: Cannot change ownership on file \"%s\"\n", ar_name);
        exit(EXIT_FAILURE);
    }

    // Change file timestamp
    int date;
    sscanf(archivedFile->header->ar_date, "%d", &date);
    struct utimbuf *datebuffer = malloc(sizeof(struct utimbuf));
    datebuffer->actime = date;
    datebuffer->modtime = date;
    if(utime(ar_name, datebuffer) == -1){
        fprintf(stderr, "Error: Cannot change timestamp on file \"%s\"\n", ar_name);
        exit(EXIT_FAILURE);
    }
    free(datebuffer);
}

void archivedFileStructPrintVerbose(archivedFileStruct *archivedFile){
    // Print file permissions
    int ar_mode;
    sscanf(archivedFile->header->ar_mode, "%d", &ar_mode);
    // **Readable ar_mode conversion print like StackOverflow solution**
    printf((ar_mode & S_IRUSR) ? "r" : "-");
    printf((ar_mode & S_IWUSR) ? "w" : "-");
    printf((ar_mode & S_IXUSR) ? "x" : "-");
    printf((ar_mode & S_IRGRP) ? "r" : "-");
    printf((ar_mode & S_IWGRP) ? "w" : "-");
    printf((ar_mode & S_IXGRP) ? "x" : "-");
    printf((ar_mode & S_IROTH) ? "r" : "-");
    printf((ar_mode & S_IWOTH) ? "w" : "-");
    printf((ar_mode & S_IXOTH) ? "x" : "-");
    printf(" ");

    // Print file owners
    printf("%s/%s\t", archivedFile->header->ar_uid, archivedFile->header->ar_gid);

    // Print file size
    printf("%s ", archivedFile->header->ar_size);

    // Print readable file last modified time
    long int ar_date;
    sscanf(archivedFile->header->ar_date, "%ld", &ar_date);
    struct tm date;
    memcpy(&date, localtime(&ar_date), sizeof(struct tm));
    printf("%s %d %02d:%02d %d ", monthName(date.tm_mon), date.tm_mday, date.tm_hour, date.tm_min, date.tm_year+1900);

    // Print file name
    printf("%s\n", archivedFile->header->ar_name);
}

char *monthName(int month){
    char *monthName;
    switch(month){
        case 0:
            monthName = "Jan";
            break;
        case 1:
            monthName = "Feb";
            break;
        case 2:
            monthName = "Mar";
            break;
        case 3:
            monthName = "Apr";
            break;
        case 4:
            monthName = "May";
            break;
        case 5:
            monthName = "Jun";
            break;
        case 6:
            monthName = "Jul";
            break;
        case 7:
            monthName = "Aug";
            break;
        case 8:
            monthName = "Sep";
            break;
        case 9:
            monthName = "Oct";
            break;
        case 10:
            monthName = "Nov";
            break;
        case 11:
            monthName = "Dec";
            break;
        default:
            break;
    }
    return monthName;
}

void dequeStructAppendArchivedFile(dequeStruct *deque, char *pathname){
    /**
     * Deque append on-disk unarchived file
     * :param deque: Archive structured data deque
     * :param pathname: On-disk unarchived file path
     * :return: None
     */
    char *filename = basename(pathname);
    if(strlen(filename) >= AR_NAME_SIZE){ // Error handling
        fprintf(stderr, "Error: Pathname \"%s\" character limit \"%d\"\n", pathname, AR_NAME_SIZE);
        exit(EXIT_FAILURE);
    }

    // Create archived file header
    archivedFileHeaderStruct *archivedFileHeader = malloc(sizeof(archivedFileHeaderStruct));
    // Fill archived file header
    struct stat filedata;
    int fd = openFileReadOnly(pathname);
    fstat(fd, &filedata);
    sprintf(archivedFileHeader->ar_name, "%s", filename);
    sprintf(archivedFileHeader->ar_date, "%ld", filedata.st_mtime);
    sprintf(archivedFileHeader->ar_uid, "%d", filedata.st_uid);
    sprintf(archivedFileHeader->ar_gid, "%d", filedata.st_gid);
    sprintf(archivedFileHeader->ar_mode, "%d", filedata.st_mode);
    sprintf(archivedFileHeader->ar_size, "%ld", filedata.st_size);
    sprintf(archivedFileHeader->ar_fmag, "%s", ARFMAG);

    // Create archived file
    archivedFileStruct *archivedFile = malloc(sizeof(archivedFileStruct));
    // Fill archived file
    archivedFile->header = archivedFileHeader;
    lseek(fd, 0, SEEK_SET);
    char *buffer = malloc(filedata.st_size);
    int bytesRead = read(fd, buffer, filedata.st_size);
    if(bytesRead != filedata.st_size){
        fprintf(stderr, "Error: Cannot read from file \"%s\"\n", pathname);
        exit(EXIT_FAILURE);
    }
    archivedFile->body = malloc(filedata.st_size*sizeof(char));
    memcpy(archivedFile->body, buffer, filedata.st_size);
    free(buffer);

    dequeAppendRear(deque, archivedFile);
    close(fd);
}

void dequeStructDeleteArchivedFile(dequeStruct *deque, char *pathname){
    /**
     * Deque delete on-disk archived file
     * :param deque: Archive structured data deque
     * :param pathname: On-disk archived file path
     * :return: None
     */
    dequeNodeStruct *cur = deque->front;
    while(cur != NULL){
        if(cur->data != NULL){
            if(strncmp(cur->data->header->ar_name, pathname, strlen(pathname)) == 0){
                cur->prev->next = cur->next;
                cur->next->prev = cur->prev;
                dequeNodeFree(cur);
                break;
            }
        }
        cur = cur->next;
    }
}
