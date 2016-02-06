#include <sys/types.h>
#include <sys/socket.h>
#include <sys/errno.h>
#include <sys/epoll.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

#define SOCKET_ERROR    -1
#define BUFFER_SIZE     512
#define HOST_NAME_SIZE  255
#define MAX_LINE_LEN    1024
#define MAX_GET         1024
#define NCONNECTIONS    20

const char *getHeaderValue(char *header);
int isWhiteSpace(const char c);
int readHeaders(int socket, int debug);
char *readLine(int socket);
void trimRight(char *str);

int main(int argc, char **argv)
{
    // Initialize variables
    int hSocket[NCONNECTIONS];   // Handles to the sockets
    struct hostent* pHostInfo;   // Holds info about a machine
    struct sockaddr_in Address;  // Internet socket address stuct
    long nHostAddress;
    char *body;
    ssize_t bytesRead;
    char strHostName[HOST_NAME_SIZE];
    int contentLength, downloadCount, c;
    int nHostPort;
    int debug = 0, verbose = 0, timesToDownload = 1;

    extern char *optarg;


    // Command Line Arguments //
    // Parse and validate the command line arguments
    if (argc < 4) {
        printf("usage: %s [-d] [-c COUNT] <host-name> <port> <file>\n", argv[0]);
        return 1;
    }
    while ((c = getopt(argc, argv, "c:dv")) != -1) {
        switch (c) {
            case 'c':
                // Download the same file multiple times
                timesToDownload = atoi(optarg);
                if (timesToDownload <= 0) {
                    fprintf(stderr, "Error: Times to download (-c COUNT) must be a positive number.\n");
                    return 1;
                }
                break;
            case 'd':
                // Print the HTTP request sent & the HTTP response headers
                debug = 1;
                break;
            case 'v':
                // Enable verbose output
                verbose = 1;
                break;
            default:
                // ?
                printf("Option -%c not recognized. Ignoring.\n", c);
                break;
        }
    }

    // Get the host name
    if (strlen(argv[optind]) >= HOST_NAME_SIZE) {
        fprintf(stderr, "Error: Host name too large. Must be less than %d characters long.\n", HOST_NAME_SIZE);
        return 1;
    }
    strcpy(strHostName, argv[optind]);

    // Get the port
    nHostPort = atoi(argv[optind + 1]);
    if (nHostPort <= 0) {
        fprintf(stderr, "Error: Invalid port number provided.\n");
        return 1;
    }


    // Setup the Address //
    // Get the IP address from name (DNS Lookup)
    pHostInfo = gethostbyname(strHostName);
    if (pHostInfo == NULL) {
        perror("DNS Lookup Failure");
        fprintf(stderr, "Error: No such hostname: %s\n", strHostName);
        return 1;
    }
    if (verbose) {
        printf("--Host: %s -> %s\n", pHostInfo->h_name, inet_ntoa(*(struct in_addr *) pHostInfo->h_addr_list[0]));
    }
    // Copy address into long
    memcpy(&nHostAddress, pHostInfo->h_addr, pHostInfo->h_length);

    // Fill address struct
    memset(&Address, 0, sizeof(Address));// Make sure it is empty first
    Address.sin_addr.s_addr = nHostAddress;
    Address.sin_port = htons(nHostPort);
    Address.sin_family = AF_INET;


    // Build the HTTP message
    char *message = malloc(MAX_GET);
    sprintf(
        message,
        "GET %s HTTP/1.1\r\nHost: %s\r\nConnection: close\r\nAccept: text/html, text/plain, */*\r\nUser-Agent: CS360Downloader/1.0\r\nReferer: http://%s/\r\n\r\n",
        argv[optind + 2],
        strHostName,
        strHostName
    );




    // Create an epoll interface
    // When a socket is ready read it
    int epollfd = epoll_create(NCONNECTIONS);

    int i;
    for (i = 0; i < NCONNECTIONS; i++) {
        // Connect to the server //
        // Make a socket
        hSocket[i] = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (hSocket[i] == SOCKET_ERROR) {
            perror("Failure to create socket");
            fprintf(stderr, "Error: Could not create connection (%d)\n", errno);
            return 2;
        }

        // Connect to host
        if (verbose) {
            printf("--Connecting to http://%s:%d\n", strHostName, nHostPort);
        }
        if (connect(hSocket[i], (struct sockaddr*) &Address, sizeof(Address)) == SOCKET_ERROR) {
            perror("Connection error");
            fprintf(stderr, "Error: Could not connect to host (%d)\n", errno);
            return 2;
        }

        struct epoll_event event;
        event.data.fd = hSocket[i];
        event.events = EPOLLIN;
        int ret = epoll_ctl(epollfd, EPOLL_CTRL_ADD, hSocket[i], &event);

        // HTTP //
        // Send HTTP to the socket
        write(hSocket[i], message, strlen(message));
        if (debug) {
            printf("Request:\n%s\n", message);
        }
     }

     for (i = 0; i < NCONNECTIONS; i++) {
         // Gets the next connection that is ready.
         struct epoll_event event;
         int nr_events = epoll_wait(epollfd, &event, 1, -1);
         int rval = read(event.data.fd, buffer, 10000);// EXAMPLE, change read to use the given file desc
         close(event.data.fd);// EXAMPLE, change close to use the event file desc

        // Read the response back from the socket
        // Read the HTTP headers
        contentLength = readHeaders(hSocket[i], debug);
        if (verbose) {
            printf("--Content Length: %d\n", contentLength);
        }

        if (contentLength >= 0) {
            // Read the body
            body = malloc((contentLength + 1) * sizeof(char));
            bytesRead = read(hSocket[i], body, contentLength);
            if (bytesRead == SOCKET_ERROR) {
                perror("Failure reading from socket");
                fprintf(stderr, "Error: Problem reading response body (%d)\n", errno);
                return 2;
            }

            // Print the body
            if (timesToDownload == 1) {
                body[bytesRead] = 0;// Make sure the body is null terminated
                printf("%s\n", body);
            }
        } else {
            if (verbose) {
                printf("--Reading to the end of the socket.\n");
            }

            // Read as much as we can from the socket
            body = malloc(BUFFER_SIZE * sizeof(char));
            while ((bytesRead = read(hSocket[i], body, BUFFER_SIZE)) != 0) {
                if (bytesRead == SOCKET_ERROR) {
                    perror("Failure reading from socket");
                    fprintf(stderr, "Error: Problem reading response body (%d)\n", errno);
                    return 2;
                }
                // Set the last character to null
                body[bytesRead] = 0;

                // Print the body
                if (timesToDownload == 1) {
                    printf("%s", body);
                }

                // Check if we've reached the end of the socket stream
                if (bytesRead < BUFFER_SIZE) {
                    break;
                }
            }
            printf("\n");
        }

        // End Connection //
        if (close(hSocket[i]) == SOCKET_ERROR) {
            perror("Failed to close socket");
            fprintf(stderr, "Error: Could not close connection (%d)\n", errno);
            return 2;
        }
    }

    return 0;
}

const char *getHeaderValue(char *header)
{
    int found = 0;
    char *c;
    for (c = header; *c != 0; c++) {
        if (found) {
            if (*c != ' ') {
                return c;
            }
        } else if (*c == ':') {
            found = 1;
        }
    }
    return NULL;
}

int isWhiteSpace(const char c)
{
    switch (c) {
        case '\r': // Fall through
        case '\n':
        case '\t':
        case '\v':
        case '\f':
        case ' ':
        case '\0':
            return 1;
        default:
            return 0;
    }
}

int readHeaders(int socket, int debug)
{
    char *line;
    int contentLength = -1;

    if (debug) {
        printf("Response:\n");
    }

    line = readLine(socket);
    while (strlen(line) != 0) {
        if (strstr(line, "Content-Length")) {
            contentLength = atoi(getHeaderValue(line));
        }
        if (debug) {
            printf("%s\n", line);
        }

        free(line);
        line = readLine(socket);
    }
    free(line);

    if (debug) {
        printf("\n");
    }
    return contentLength;
}

/**
 * Read from the socket until a '\n' characters is found.
 */
char *readLine(int socket)
{
    char tline[MAX_LINE_LEN];
    char *line;

    int lineLength = 0;
    ssize_t charactersRead = 0;

    while((charactersRead = read(socket, tline + lineLength, 1)) < MAX_LINE_LEN) {
        if (charactersRead == SOCKET_ERROR) {
            perror("Failure reading from socket");
            fprintf(stderr, "Error: Problem reading HTTP headers (%d)\n", errno);
            exit(2);
        }
        lineLength += charactersRead;
        if (tline[lineLength - 1] == '\n') {
            break;
        }
    }
    tline[lineLength] = 0;

    trimRight(tline);
    line = (char *) malloc((strlen(tline) + 1) * sizeof(char));
    strcpy(line, tline);
    return line;
}

void trimRight(char *str)
{
    size_t len = strlen(str);
    while (len >= 0 && isWhiteSpace(str[len])) {
        str[len--] = 0;
    }
}
