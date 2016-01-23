#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <unistd.h>
#include <netdb.h>
#include <string.h>
#include <sys/errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include "http.h"

#define SOCKET_ERROR        -1
#define QUEUE_SIZE          5
#define MAX_LINE_LENGTH     MAX_HEADER_LEN * 2
#define MAX_NUM_HEADERS     50

int getLine(int socket, char *buffer, int i);

int readHeaders(int socket, struct header *headers);

char *trimLeft(char *str);

int writeError(int socket, int responseCode);

int main(int argc, char **argv)
{
    int port;
    char *webDirectory;
    int hSocket, hServerSocket;  /* handle to socket */
    struct sockaddr_in Address; /* Internet socket address stuct */
    int nAddressSize = sizeof(struct sockaddr_in);
    char pBuffer[MAX_LINE_LENGTH + 1];
    int opt, i;
    int verbose = 0;
    int numHeaders = 0;
    struct header headers[MAX_NUM_HEADERS];

    // Command Line Arguments //
    if (argc < 3) {
        printf("usage: %s [-v] <port> <dir>\n", argv[0]);
        return 1;
    }
    while ((opt = getopt(argc, argv, "v")) != -1) {
        switch (opt) {
            case 'v':
                // Enable verbose output
                verbose = 1;
                break;
            default:
                // ?
                printf("Option -%c not recognized. Ignoring.\n", opt);
                break;
        }
    }

    port = atoi(argv[optind]);
    webDirectory = argv[optind + 1];

    if (port <= 0) {
        printf("Invalid port provided.\n");
        return 1;
    }

    // Initialize the Server //
    // Create the socket
    if (verbose) {
        printf(" - Creating socket to listen with.\n");
    }
    hServerSocket = socket(AF_INET, SOCK_STREAM, 0);

    if(hServerSocket == SOCKET_ERROR) {
        perror("Could not make a socket");
        return 2;
    }

    // Fill in the address and bind to the port
    memset(&Address, 0, sizeof(Address));// Make sure it is empty first
    Address.sin_addr.s_addr = INADDR_ANY;
    Address.sin_port = htons(port);
    Address.sin_family = AF_INET;

    if (verbose) {
        printf(" - Binding to port %d\n", port);
    }
    if (bind(hServerSocket, (struct sockaddr*) &Address, sizeof(Address)) == SOCKET_ERROR) {
        perror("Failure binding to port");
        return 2;
    }
    // Update the socket address
    if (getsockname(hServerSocket, (struct sockaddr *) &Address, (socklen_t *) &nAddressSize) == SOCKET_ERROR) {
        perror("Failed to get the socket name");
        return 2;
    }
    if (verbose) {
        printf(" - Socket opened and bound. File Descriptor: %d\n", hServerSocket);
    }

    // Initialize the listening queue
    if (verbose) {
        printf(" - Making a listen queue of %d elements.\n", QUEUE_SIZE);
    }
    if (listen(hServerSocket, QUEUE_SIZE) == SOCKET_ERROR) {
        perror("Failed to listen to the socket");
        return 2;
    }

    for (;;) {
        // Listen on the socket
        if (verbose) {
            printf(" - Waiting for a connection.\n");
        }
        hSocket = accept(hServerSocket, (struct sockaddr*) &Address, (socklen_t *) &nAddressSize);
        if (verbose) {
            printf(" - Got a connection from %X (%d)\n", Address.sin_addr.s_addr, ntohs(Address.sin_port));
        }

        // Read in the response
        if (getLine(hSocket, pBuffer, MAX_LINE_LENGTH) == SOCKET_ERROR) {
            printf("Failed to read HTTP request first line.\n");
            writeError(hSocket, HTTP_BADREQUEST);
        } else if ((numHeaders = readHeaders(hSocket, headers)) == SOCKET_ERROR) {
            printf("Error reading the headers.\n");
            writeError(hSocket, HTTP_BADREQUEST);
        } else {
            // Read all the headers successfully, respond to the request.
            if (verbose) {
                printf("HTTP Headers Received:\n%s\n", pBuffer);
                for (i = 0; i < numHeaders; i++) {
                    printf("%s: %s\n", headers[i].key, headers[i].value);
                }
            }
            // TODO
            writeError(hSocket, HTTP_BADREQUEST);
        }

        // Close the socket
        if (verbose) {
            printf(" - Closing connection.\n");
        }
        if (close(hSocket) == SOCKET_ERROR) {
            perror("Failed to close the socket connection");
            return 2;
        }
    }

    return 0;
}

int writeError(int socket, int responseCode)
{
    char response[MAX_LINE_LENGTH];
    sprintf(response, "HTTP/1.1 %d %s\r\nConnection: close\r\n\r\n", responseCode, "Bad Request");

    write(socket, response, strlen(response));
    return 0;
}

int readHeaders(int socket, struct header *headers)
{
    int headersReadIn = 0;
    char line[MAX_LINE_LENGTH];
    char *loc;

    if (getLine(socket, line, MAX_LINE_LENGTH) == SOCKET_ERROR) {
        return SOCKET_ERROR;
    }
    while (strlen(line) != 0) {
        loc = strchr(line, ':');
        if (loc == NULL) {
            printf("Bad header found. Ignoring. Header: %s\n", line);
        } else {
            *loc = 0;
            if (strlen(line) > MAX_HEADER_LEN) {
                return SOCKET_ERROR;
            }
            strcpy(headers[headersReadIn].key, line);

            loc++;

            loc = trimLeft(loc);
            if (strlen(loc) > MAX_HEADER_LEN) {
                return SOCKET_ERROR;
            }
            strcpy(headers[headersReadIn].value, loc);

            headersReadIn++;
        }

        if (getLine(socket, line, MAX_LINE_LENGTH) == SOCKET_ERROR) {
            return SOCKET_ERROR;
        }
    }

    return headersReadIn;
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

char *trimLeft(char *str)
{
    while (str != 0 && isWhiteSpace(*str)) {
        str++;
    }
    return str;
}

void trimRight(char *str)
{
    size_t len = strlen(str);
    while (len >= 0 && isWhiteSpace(str[len])) {
        str[len--] = 0;
    }
}

int getLine(int socket, char *buffer, int bufferSize)
{
    int lineLength = 0;
    ssize_t charactersRead = 0;

    // Read until a new line character is read in
    while ((charactersRead = read(socket, buffer + lineLength, 1)) < bufferSize) {
        if (charactersRead == SOCKET_ERROR) {
            perror("Failure reading from socket");
            printf("Error: Problem reading line (%d)\n", errno);
            return SOCKET_ERROR;
        }
        lineLength += charactersRead;
        if (buffer[lineLength - 1] == '\n') {
            break;
        }
    }

    // Mark the end of the string and trim any trailing whitespace
    buffer[lineLength] = 0;
    trimRight(buffer);

    return 0;
}