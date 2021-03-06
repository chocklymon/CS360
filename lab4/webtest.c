#define OS_X 0

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/errno.h>
#if OS_X == 1
#include "epoll.h"
#else
#include <sys/epoll.h>
#endif
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#define SOCKET_ERROR    -1
#define BUFFER_SIZE     512
#define HOST_NAME_SIZE  255
#define MAX_LINE_LEN    1024
#define MAX_GET         1024
#define USEC_PER_SEC   (double) 1000000
#define FD_OFFSET 4
#define TRUE 1
#define FALSE 0

const char *getHeaderValue(char *header);
int isWhiteSpace(const char c);
int readHeaders(int socket, int debug);
char *readLine(int socket);
void trimRight(char *str);

int main(int argc, char **argv)
{
    // Initialize variables
    struct hostent* pHostInfo;   // Holds info about a machine
    struct sockaddr_in Address;  // Internet socket address stuct
    long nHostAddress;
    char *body;
    ssize_t bytesRead;
    char strHostName[HOST_NAME_SIZE];
    double usec;
    int contentLength;
    int nHostPort;
    int hSocket, epollfd;
    int count = 1;
    int debug = FALSE, verbose = FALSE, veryVerbose = FALSE;
    int i, c, rval, index;

    extern char *optarg;


    // Command Line Arguments //
    // Parse and validate the command line arguments
    if (argc < 5) {
        printf("usage: %s [-d] [-v|w] <host-name> <port> <path> <count>\n", argv[0]);
        return 1;
    }
    while ((c = getopt(argc, argv, "dvw")) != -1) {
        switch (c) {
            case 'd':
                // Print the times for each response
                debug = TRUE;
                break;
            case 'v':
                // Enable verbose output
                verbose = TRUE;
                break;
            case 'w':
                verbose = TRUE;
                veryVerbose = TRUE;
                break;
            default:
                // ?
                fprintf(stderr, "Option -%c not recognized. Ignoring.\n", c);
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

    // Get the count
    count = atoi(argv[optind + 3]);
    if (count <= 0) {
        fprintf(stderr, "Error: Invalid count provided. Count must be a positive number.\n");
    }

    // Build variables the depend on the count variable
    int hSockets[count];   // Handles to the sockets
    double timings[count];
    struct timeval oldtime[count];


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
        "GET %s HTTP/1.1\r\nHost: %s\r\nConnection: close\r\nAccept: text/html, text/plain, */*\r\nUser-Agent: CS360WebTest/1.0\r\nReferer: http://%s/\r\n\r\n",
        argv[optind + 2],
        strHostName,
        strHostName
    );
    if (veryVerbose) {
        printf("Request:\n%s\n", message);
    }

    // Create an epoll interface
    epollfd = epoll_create(count);

    if (verbose) {
        printf("--Creating sockets.\n");
    }
    for (i = 0; i < count; i++) {
        // Connect to the server //
        // Make a socket
        hSockets[i] = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (hSockets[i] == SOCKET_ERROR) {
            perror("Failure to create socket");
            fprintf(stderr, "Error: Could not create connection (%d)\n", errno);
            return 2;
        }
    }

    if (verbose) {
        printf("--Connecting to http://%s:%d %d times.\n", strHostName, nHostPort, count);
    }
    for (i = 0; i < count; i++) {
        // Connect to host
        if (connect(hSockets[i], (struct sockaddr*) &Address, sizeof(Address)) == SOCKET_ERROR) {
            perror("Connection error");
            fprintf(stderr, "Error: Could not connect to host (%d)\n", errno);
            return 2;
        }

        // HTTP //
        // Send HTTP to the socket
        write(hSockets[i], message, strlen(message));


        // Store the event
        struct epoll_event event;
        event.data.fd = hSockets[i];
        event.events = EPOLLIN;
        int ret = epoll_ctl(epollfd, EPOLL_CTL_ADD, hSockets[i], &event);
        if (ret < 0) {
            perror("Epoll Control Add");
        }

        // Store the request start time
        gettimeofday(&oldtime[i], NULL);

        if (verbose) {
            printf("--(%02d) Created socket #%02d\n", i, hSockets[i]);
        }
    }
    free(message);

    if (verbose) {
        printf("--Handling sockets.\n");
    }
    for (i = 0; i < count; i++) {
        // Gets the next connection that is ready.
        struct epoll_event event;
        rval = epoll_wait(epollfd, &event, 1, -1);
        if (rval < 0) {
            perror("Epoll Wait");
        }
        index = event.data.fd - FD_OFFSET;
        hSocket = event.data.fd;
        if (verbose) {
            printf("--(%02d) Handling socket %02d (Index: %d)\n", i, hSocket, index);
        }

        // Read the response back from the socket
        // Read the HTTP headers
        contentLength = readHeaders(hSocket, veryVerbose);
        if (verbose) {
            printf("--Content Length: %d\n", contentLength);
        }

        if (contentLength >= 0) {
            // Read the body
            body = malloc((contentLength + 1) * sizeof(char));
            bytesRead = read(hSocket, body, contentLength);
            if (bytesRead == SOCKET_ERROR) {
                perror("Failure reading from socket");
                fprintf(stderr, "Error: Problem reading response body (%d)\n", errno);
                return 2;
            }

            // Print the body
            if (count == 1) {
                body[bytesRead] = 0;// Make sure the body is null terminated
                printf("%s\n", body);
            }
        } else {
            if (verbose) {
                printf("--Reading to the end of the socket.\n");
            }

            // Read as much as we can from the socket
            body = malloc(BUFFER_SIZE * sizeof(char));
            while ((bytesRead = read(hSocket, body, BUFFER_SIZE)) != 0) {
                if (bytesRead == SOCKET_ERROR) {
                    perror("Failure reading from socket");
                    fprintf(stderr, "Error: Problem reading response body (%d)\n", errno);
                    return 2;
                }
                // Set the last character to null
                body[bytesRead] = 0;

                // Print the body
                if (count == 1) {
                    printf("%s", body);
                }

                // Check if we've reached the end of the socket stream
                if (bytesRead < BUFFER_SIZE) {
                    break;
                }
            }
            printf("\n");
        }

        // Report on the timings
        struct timeval newtime;
        // Get the current time and subtract the starting time for this request.
        gettimeofday(&newtime, NULL);
        usec = (newtime.tv_sec - oldtime[index].tv_sec) * USEC_PER_SEC
                      + (newtime.tv_usec - oldtime[index].tv_usec);
        timings[i] = usec;
        if (debug) {
            printf("Time: %f seconds\n", usec / USEC_PER_SEC);
        }

        // Remove this socket from being epolled
        epoll_ctl(epollfd, EPOLL_CTL_DEL, hSocket, &event);
    }

    if (verbose) {
        printf("--Closing socket connections.\n");
    }
    for (i = 0; i < count; i++) {
        // End Connection //
        if (close(hSockets[i]) == SOCKET_ERROR) {
            perror("Failed to close socket");
            fprintf(stderr, "Error: Could not close connection (%d)\n", errno);
            return 2;
        }
    }

    // Generate the report //
    if (verbose) {
        printf("--Generating report.\n");
    }
    double mean = 0.0, variance = 0.0, standardDeviation = 0.0;

    // Find the average time
    for (i = 0; i < count; i++) {
        mean += timings[i];
    }
    mean = mean / (double) count;

    // Find the standard deviation
    for (i = 0; i < count; i++) {
        variance += pow(timings[i] - mean, 2);
    }
    variance = variance / (double) count;
    standardDeviation = sqrt(variance);

    // Find the standa
    printf("Average response time: %f seconds.\n", mean / USEC_PER_SEC);
    printf("Standard Deviation: %f seconds.\n", standardDeviation / USEC_PER_SEC);


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
