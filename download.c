#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#define MAX_BUFFER 1024
#define FTP_PORT 21

// Structure to store parsed URL information
typedef struct {
    char user[128];
    char password[128];
    char host[256];
    char path[512];
} URLInfo;

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
    
    printf("Parsed URL:\n");
    printf("  User: %s\n", info->user);
    printf("  Password: %s\n", info->password);
    printf("  Host: %s\n", info->host);
    printf("  Path: %s\n", info->path);
    
    return 0;
}

// Reused from getip.c - Function to get IP address from hostname
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

// Reused and adapted from clientTCP.c - Function to connect to server
int connectToServer(const char *ip, int port) {
    int sockfd;
    struct sockaddr_in server_addr;
    
    /*server address handling*/
    bzero((char *)&server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(ip);    /*32 bit Internet address network byte ordered*/
    server_addr.sin_port = htons(port);              /*server TCP port must be network byte ordered */
    
    /*open a TCP socket*/
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket()");
        return -1;
    }
    
    /*connect to the server*/
    if (connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("connect()");
        close(sockfd);
        return -1;
    }
    
    return sockfd;
}

// Function to read FTP response
int readResponse(int sockfd, char *buffer, int size) {
    int bytes = 0;
    int total = 0;
    char c;
    
    memset(buffer, 0, size);
    
    // Read until we get a complete response
    // FTP responses end with code + space + text + CRLF
    while (total < size - 1) {
        bytes = read(sockfd, &c, 1);
        if (bytes <= 0) break;
        
        buffer[total++] = c;
        
        // Check if we have a complete line
        if (total >= 2 && buffer[total-2] == '\r' && buffer[total-1] == '\n') {
            // Check if this is the final line (code followed by space)
            if (total >= 4 && buffer[3] == ' ') {
                break;
            }
        }
    }
    
    buffer[total] = '\0';
    printf("< %s", buffer);
    
    return total;
}

// Function to send FTP command
int sendCommand(int sockfd, const char *cmd, const char *arg) {
    char buffer[MAX_BUFFER];
    size_t bytes;
    
    if (arg != NULL) {
        snprintf(buffer, sizeof(buffer), "%s %s\r\n", cmd, arg);
    } else {
        snprintf(buffer, sizeof(buffer), "%s\r\n", cmd);
    }
    
    printf("> %s", buffer);
    
    /*send command to the server*/
    bytes = write(sockfd, buffer, strlen(buffer));
    if (bytes > 0) {
        // Command sent successfully
    } else {
        perror("write()");
        return -1;
    }
    
    return 0;
}

// Function to parse PASV response to get IP and port
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

// Function to download file
int downloadFile(int sockfd, const char *filename) {
    char buffer[MAX_BUFFER];
    int bytes;
    FILE *file;
    int total = 0;
    
    // Extract just the filename for saving
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
    
    // Read data and write to file (similar to clientTCP read pattern)
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
    
    // Check arguments
    if (argc != 2) {
        fprintf(stderr, "Usage: %s ftp://[user:password@]host/path\n", argv[0]);
        fprintf(stderr, "Example: %s ftp://ftp.up.pt/pub/gnu/emacs/elisp-manual-21-2.8.tar.gz\n", argv[0]);
        exit(-1);
    }
    
    // Parse URL
    if (parseURL(argv[1], &urlInfo) < 0) {
        exit(-1);
    }
    
    // Get IP address (reusing getip.c logic)
    if (getIPAddress(urlInfo.host, ip) < 0) {
        exit(-1);
    }
    
    // Connect to FTP server - control connection (reusing clientTCP.c logic)
    printf("\n--- Connecting to FTP server ---\n");
    control_sock = connectToServer(ip, FTP_PORT);
    if (control_sock < 0) {
        exit(-1);
    }
    
    // Read welcome message
    readResponse(control_sock, buffer, sizeof(buffer));
    if (buffer[0] != '2') {
        fprintf(stderr, "Error: Failed to connect to server\n");
        close(control_sock);
        exit(-1);
    }
    
    // Send USER command
    printf("\n--- Authentication ---\n");
    sendCommand(control_sock, "USER", urlInfo.user);
    readResponse(control_sock, buffer, sizeof(buffer));
    
    // Send PASS command
    sendCommand(control_sock, "PASS", urlInfo.password);
    readResponse(control_sock, buffer, sizeof(buffer));
    if (buffer[0] != '2') {
        fprintf(stderr, "Error: Authentication failed\n");
        close(control_sock);
        exit(-1);
    }
    
    // Enter passive mode
    printf("\n--- Entering Passive Mode ---\n");
    sendCommand(control_sock, "PASV", NULL);
    readResponse(control_sock, buffer, sizeof(buffer));
    if (buffer[0] != '2') {
        fprintf(stderr, "Error: PASV command failed\n");
        close(control_sock);
        exit(-1);
    }
    
    // Parse PASV response
    if (parsePasvResponse(buffer, data_ip, &data_port) < 0) {
        close(control_sock);
        exit(-1);
    }
    
    // Connect to data port (reusing clientTCP.c connection logic)
    printf("\n--- Opening Data Connection ---\n");
    data_sock = connectToServer(data_ip, data_port);
    if (data_sock < 0) {
        close(control_sock);
        exit(-1);
    }
    
    // Request file transfer
    printf("\n--- Requesting File Transfer ---\n");
    sendCommand(control_sock, "RETR", urlInfo.path);
    readResponse(control_sock, buffer, sizeof(buffer));
    if (buffer[0] != '1') {
        fprintf(stderr, "Error: RETR command failed\n");
        close(data_sock);
        close(control_sock);
        exit(-1);
    }
    
    // Download file
    printf("\n--- Downloading File ---\n");
    if (downloadFile(data_sock, urlInfo.path) < 0) {
        close(data_sock);
        close(control_sock);
        exit(-1);
    }
    
    // Close data connection
    if (close(data_sock) < 0) {
        perror("close()");
    }
    
    // Read transfer complete message
    readResponse(control_sock, buffer, sizeof(buffer));
    
    // Send QUIT command
    printf("\n--- Closing Connection ---\n");
    sendCommand(control_sock, "QUIT", NULL);
    readResponse(control_sock, buffer, sizeof(buffer));
    
    // Close control connection
    if (close(control_sock) < 0) {
        perror("close()");
        exit(-1);
    }
    
    printf("\n=== Download Successful ===\n");
    
    return 0;
}

