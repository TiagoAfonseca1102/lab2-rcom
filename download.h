#ifndef DOWNLOAD_H
#define DOWNLOAD_H

#include <stdio.h>

#define MAX_BUFFER 1024
#define FTP_PORT 21

// Structure to store parsed URL information
typedef struct {
    char user[128];
    char password[128];
    char host[256];
    char path[512];
    char directory[512];
    char filename[256];
} URLInfo;

// URL parsing
int parseURL(const char *url, URLInfo *info);

// DNS
int getIPAddress(const char *hostname, char *ip);

// Connections
int connectToServer(const char *ip, int port);

// FTP communication
int readResponse(int sockfd, char *buffer, int size);
int sendCommand(int sockfd, const char *cmd, const char *arg);
int sendCommandCWD(int sockfd, const char *dir);

// Response utilities
int isSuccessCode(int code);
int isErrorCode(int code);
const char* getErrorMessage(int code);

// PASV parsing
int parsePasvResponse(const char *response, char *ip, int *port);

//  File transfer
int downloadFile(int sockfd, const char *filename);

#endif // DOWNLOAD_H

