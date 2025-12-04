#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include "download.h"


// Function to parse FTP URL
// Format: ftp://[user:password@]host/path
int parseURL(const char *url, URLInfo *info) {
    char temp[1024];
    char *ptr;
    
    // Check if URL starts with ftp://
    if (strncmp(url, "ftp://", 6) != 0) {
        fprintf(stderr, "Error: URL must start with ftp://\n");
        return -1;
    }
    
    strcpy(temp, url + 6); // Skip "ftp://"
    
    // Check for user:password@
    ptr = strchr(temp, '@');
    if (ptr != NULL) {
        *ptr = '\0';
        char *colon = strchr(temp, ':');
        if (colon != NULL) {
            *colon = '\0';
            strcpy(info->user, temp);
            strcpy(info->password, colon + 1);
        } else {
            strcpy(info->user, temp);
            strcpy(info->password, "");
        }
        strcpy(temp, ptr + 1);
    } else {
        strcpy(info->user, "anonymous");
        strcpy(info->password, "anonymous@");
    }
    
    // Extract host and path
    ptr = strchr(temp, '/');
    if (ptr != NULL) {
        *ptr = '\0';
        strcpy(info->host, temp);
        strcpy(info->path, ptr + 1);
    } else {
        strcpy(info->host, temp);
        strcpy(info->path, "");
    }
    
    // Separate directory and filename
    char *last_slash = strrchr(info->path, '/');
    if (last_slash != NULL && last_slash != info->path) {
        // Has directory
        size_t dir_len = last_slash - info->path;
        strncpy(info->directory, info->path, dir_len);
        info->directory[dir_len] = '\0';
        strcpy(info->filename, last_slash + 1);
    } else {
        // No directory, just filename
        strcpy(info->directory, "");
        if (strlen(info->path) > 0) {
            strcpy(info->filename, info->path);
        } else {
            strcpy(info->filename, "");
        }
    }
    
    printf("Parsed URL:\n");
    printf("User: %s\n", info->user);
    printf("Password: %s\n", info->password);
    printf("Host: %s\n", info->host);
    printf("Path: %s\n", info->path);
    printf("Directory: %s\n", info->directory);
    printf("Filename: %s\n", info->filename);
    
    return 0;
}

// get IP address from hostname
int getIPAddress(const char *hostname, char *ip) {
    struct hostent *h;
    
    if ((h = gethostbyname(hostname)) == NULL) {
        herror("gethostbyname()");
        return -1;
    }
    
    strcpy(ip, inet_ntoa(*((struct in_addr *)h->h_addr)));
    printf("Host %s has IP address %s\n", hostname, ip);
    
    return 0;
}

// Function to connect to server
int connectToServer(const char *ip, int port) {
    int sockfd;
    struct sockaddr_in server_addr;
    
    // server address handling
    bzero((char *)&server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(ip);
    server_addr.sin_port = htons(port);
    
    // open a TCP socket
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket()");
        return -1;
    }
    
    // connect to the server
    if (connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("connect()");
        close(sockfd);
        return -1;
    }
    
    return sockfd;
}

// Function to read FTP response 
// Returns the response code
int readResponse(int sockfd, char *buffer, int size) {
    memset(buffer, 0, size);
    int total = 0;
    char line[MAX_BUFFER];
    int code = 0;
    int first_line = 1;
    
    while (1) {
        int i = 0;
        char c;
        
        // Read one line at a time
        while (i < MAX_BUFFER - 1) {
            int n = read(sockfd, &c, 1);
            if (n <= 0) {
                if (total > 0) return code;
                return -1;
            }
            
            line[i++] = c;
            
            // Check for end of line 
            if (i >= 2 && line[i-2] == '\r' && line[i-1] == '\n') {
                line[i] = '\0';
                break;
            }
        }
        
        // Add line to the buffer
        if (total + i < size) {
            strcat(buffer, line);
            total += i;
        }
        
        // Parse response code from first line
        if (first_line && i >= 3) {
            code = (line[0] - '0') * 100 + (line[1] - '0') * 10 + (line[2] - '0');
            first_line = 0;
        }
        
        // Check if this is the last line
        // Last line format: "XXX " code + space
        if (i >= 4 && line[0] == (code/100 + '0') && 
            line[1] == ((code/10)%10 + '0') && 
            line[2] == (code%10 + '0') && 
            line[3] == ' ') {
            break;
        }
    }
    
    printf("< %s", buffer);
    return code;
}

// check if response code indicates success
int isSuccessCode(int code) {
    return (code >= 200 && code < 300);
}

// check if response code indicates error
int isErrorCode(int code) {
    return (code >= 400);
}

// get error message based on code
const char* getErrorMessage(int code) {
    if (code >= 500) return "Permanent error";
    if (code >= 400) return "Temporary error";
    return "Unknown error";
}

// send FTP command
int sendCommand(int sockfd, const char *cmd, const char *arg) {
    char buffer[MAX_BUFFER];
    size_t bytes;
    
    if (arg != NULL && strlen(arg) > 0) {
        snprintf(buffer, sizeof(buffer), "%s %s\r\n", cmd, arg);
    } else {
        snprintf(buffer, sizeof(buffer), "%s\r\n", cmd);
    }
    
    printf("> %s", buffer);
    
    bytes = write(sockfd, buffer, strlen(buffer));
    if (bytes != strlen(buffer)) {
        perror("write()");
        return -1;
    }
    
    return 0;
}

// change working directory (CWD command)
int sendCommandCWD(int sockfd, const char *dir) {
    char buffer[MAX_BUFFER];
    char cmd[MAX_BUFFER];
    
    snprintf(cmd, sizeof(cmd), "CWD %s\r\n", dir);
    
    printf("> CWD %s\n", dir);
    
    // Send command
    if (write(sockfd, cmd, strlen(cmd)) < 0) {
        perror("write()");
        return -1;
    }
    
    // Read response
    int code = readResponse(sockfd, buffer, sizeof(buffer));
    
    return code;
}

// parse PASV response to get IP and port
int parsePasvResponse(const char *response, char *ip, int *port) {
    int ip1, ip2, ip3, ip4, port1, port2;
    char *start;
    
    // Find the opening parenthesis
    start = strchr(response, '(');
    if (start == NULL) {
        fprintf(stderr, "Error: Invalid PASV response\n");
        return -1;
    }
    
    // Parse the 6 numbers
    if (sscanf(start + 1, "%d,%d,%d,%d,%d,%d", &ip1, &ip2, &ip3, &ip4, &port1, &port2) != 6) {
        fprintf(stderr, "Error: Failed to parse PASV response\n");
        return -1;
    }
    
    sprintf(ip, "%d.%d.%d.%d", ip1, ip2, ip3, ip4);
    *port = port1 * 256 + port2;
    
    printf("PASV mode: IP=%s, Port=%d\n", ip, *port);
    
    return 0;
}

// download file
int downloadFile(int sockfd, const char *filename) {
    char buffer[MAX_BUFFER];
    int bytes;
    FILE *file;
    int total = 0;
    
    // Extract filename for saving
    const char *name = strrchr(filename, '/');
    if (name != NULL) {
        name++;
    } else {
        name = filename;
    }
    
    // If no filename, use default
    if (strlen(name) == 0) {
        name = "downloaded_file";
    }
    
    file = fopen(name, "wb");
    if (file == NULL) {
        perror("fopen()");
        return -1;
    }
    
    printf("Downloading to file: %s\n", name);
    
    // Read data and write to file
    while ((bytes = read(sockfd, buffer, sizeof(buffer))) > 0) {
        fwrite(buffer, 1, bytes, file);
        total += bytes;
    }
    
    fclose(file);
    printf("Download complete: %d bytes received\n", total);
    
    return total;
}

int main(int argc, char **argv) {
    URLInfo urlInfo;
    char ip[32];
    int control_sock, data_sock;
    char buffer[MAX_BUFFER];
    char data_ip[32];
    int data_port;
    int code;
    
    // Check arguments
    if (argc != 2) {
        fprintf(stderr, "Usage: %s ftp://[user:password@]host/path\n", argv[0]);
        fprintf(stderr, "Example: %s ftp://ftp.up.pt/pub/CPAN/README.html\n", argv[0]);
        exit(-1);
    }
    
    // Parse URL
    if (parseURL(argv[1], &urlInfo) < 0) {
        exit(-1);
    }
    
    // Get IP address
    if (getIPAddress(urlInfo.host, ip) < 0) {
        exit(-1);
    }
    
    // Connect to FTP server - control connection
    printf("\n--- Connecting to FTP server ---\n");
    control_sock = connectToServer(ip, FTP_PORT);
    if (control_sock < 0) {
        exit(-1);
    }
    
    // Read welcome message
    code = readResponse(control_sock, buffer, sizeof(buffer));
    if (!isSuccessCode(code)) {
        fprintf(stderr, "Error: Server didn't send welcome (code %d)\n", code);
        close(control_sock);
        exit(-1);
    }
    
    // autenticaÃ§Ã£o
    printf("\n--- Authentication ---\n");
    
    // Send USER command
    sendCommand(control_sock, "USER", urlInfo.user);
    code = readResponse(control_sock, buffer, sizeof(buffer));
    
    if (isErrorCode(code)) {
        fprintf(stderr, "Error: USER command failed (code %d): %s\n", 
                code, getErrorMessage(code));
        close(control_sock);
        exit(-1);
    }
    
    // Send PASS command
    sendCommand(control_sock, "PASS", urlInfo.password);
    code = readResponse(control_sock, buffer, sizeof(buffer));
    
    if (!isSuccessCode(code)) {
        fprintf(stderr, "Error: Authentication failed (code %d)\n", code);
        close(control_sock);
        exit(-1);
    }
    
    // cwd
    if (strlen(urlInfo.directory) > 0) {
        printf("\n--- Changing Directory ---\n");
        code = sendCommandCWD(control_sock, urlInfo.directory);
        
        if (!isSuccessCode(code)) {
            fprintf(stderr, "Warning: CWD failed (code %d), will try full path in RETR\n", code);
            // Does not exit -> try with full path in RETR
            strcpy(urlInfo.filename, urlInfo.path);
        } else {
            printf("Changed to directory: %s\n", urlInfo.directory);
        }
    }
    
    // set binary
    printf("\n--- Setting Binary Mode ---\n");
    sendCommand(control_sock, "TYPE", "I");
    code = readResponse(control_sock, buffer, sizeof(buffer));
    
    if (!isSuccessCode(code)) {
        fprintf(stderr, "Warning: TYPE I failed (code %d), continuing anyway\n", code);
    }
    
    // passive mode
    printf("\n--- Entering Passive Mode ---\n");
    sendCommand(control_sock, "PASV", NULL);
    code = readResponse(control_sock, buffer, sizeof(buffer));
    
    if (!isSuccessCode(code)) {
        fprintf(stderr, "Error: PASV command failed (code %d)\n", code);
        close(control_sock);
        exit(-1);
    }
    
    // Parse PASV response
    if (parsePasvResponse(buffer, data_ip, &data_port) < 0) {
        close(control_sock);
        exit(-1);
    }
    
    // open data connection
    printf("\n--- Opening Data Connection ---\n");
    data_sock = connectToServer(data_ip, data_port);
    if (data_sock < 0) {
        close(control_sock);
        exit(-1);
    }
    
    // request file
    printf("\n--- Requesting File Transfer ---\n");
    sendCommand(control_sock, "RETR", urlInfo.filename);
    code = readResponse(control_sock, buffer, sizeof(buffer));
    
    if (code < 100 || code >= 200) {
        fprintf(stderr, "Error: RETR command failed (code %d)\n", code);
        fprintf(stderr, "Server response: %s\n", buffer);
        close(data_sock);
        close(control_sock);
        exit(-1);
    }
    
    // download of the file
    printf("\n--- Downloading File ---\n");
    if (downloadFile(data_sock, urlInfo.filename) < 0) {
        close(data_sock);
        close(control_sock);
        exit(-1);
    }
    
    // Close data connection
    close(data_sock);
    
    // Read transfer complete message
    code = readResponse(control_sock, buffer, sizeof(buffer));
    if (!isSuccessCode(code)) {
        fprintf(stderr, "Warning: Transfer completion returned code %d\n", code);
    }
    
    // exit
    printf("\n--- Closing Connection ---\n");
    sendCommand(control_sock, "QUIT", NULL);
    code = readResponse(control_sock, buffer, sizeof(buffer));
    
    // Close control connection
    close(control_sock);
    
    printf("\n --- Download Successful ---\n");
    
    return 0;
}

