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
#include "server.h"
#include "http.h"
#include "utils.h"

int writeError(int socket, int statusCode);

int writeResponse(int socket, int statusCode, Header **headers, int numHeaders, char content[]);

void writeHeaders(int socket, int statusCode, Header **headers, int numHeaders);

int main(int argc, char **argv)
{
    int port;
    char *webDirectory;
    int hSocket, hServerSocket;  /* handle to socket */
    struct sockaddr_in Address; /* Internet socket address stuct */
    int nAddressSize = sizeof(struct sockaddr_in);
    char pBuffer[MAX_LINE_LENGTH + 1];
    int opt, i, result;
    int verbose = 0;
    int numHeaders = 0;
    Header **inputHeaders = malloc(MAX_NUM_HEADERS * sizeof(Header*));

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
            writeError(hSocket, HTTP_INTERNAL_SERVER_ERROR);
        } else if ((result = readHeaders(hSocket, inputHeaders, &numHeaders, MAX_NUM_HEADERS)) <= SOCKET_ERROR) {
            printf("Error reading the headers (%d).\n", numHeaders);
            if (result == HEADER_ERROR) {
                // Problem parsing the headers
                writeError(hSocket, HTTP_BAD_REQUEST);
            } else {
                // Problem reading from the socket, or other error
                writeError(hSocket, HTTP_INTERNAL_SERVER_ERROR);
            }
        } else {
            // Read all the headers successfully, respond to the request.
            if (verbose) {
                printf("HTTP Headers Received:\n%s\n", pBuffer);
                for (i = 0; i < numHeaders; i++) {
                    printf("%s: %s\n", inputHeaders[i]->key, inputHeaders[i]->value);
                }
            }
            // TODO
            writeError(hSocket, HTTP_NOT_IMPLEMENTED);
        }

        freeHeaders(inputHeaders, numHeaders);

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

int writeError(int socket, int statusCode)
{
    char response[MAX_LINE_LENGTH];
    sprintf(response, "HTTP/1.1 %d %s\r\nConnection: close\r\n\r\n", statusCode, getStatusCodeName(statusCode));

    write(socket, response, strlen(response));
    return 0;
}

//int writeError(int socket, int statusCode)
//{
//    char responseBody[MAX_LINE_LENGTH];
//    char* statusCodeName = getStatusCodeName(statusCode);
//    sprintf(
//        responseBody,
//        "<!DOCTYPE html>\n<html lang=\"en\">\n<head><title>Error - %s</title></head>\n<body><h1>Error %d</h1><p>%s</p></body></html>",
//        statusCodeName,
//        statusCode,
//        statusCodeName
//    );
//    struct header headers[2];
//    strcpy(headers[0].key, "Connection");
//    strcpy(headers[0].value, "close");
//    strcpy(headers[1].key, "Content-Type");
//    strcpy(headers[1].value, "text/html");
//    writeResponse(socket, statusCode, headers, 2, responseBody);
//    return 0;
//}
//
//int writeResponse(int socket, int statusCode, Header **headers, int numHeaders, char content[])
//{
//    char response[MAX_LINE_LENGTH * 2];
//    sprintf(response, "%d", strlen(content));
//
//    // Add the the headers
//    struct header allHeaders[numHeaders + 1];
//    memcpy(allHeaders, headers, sizeof(headers));
//    strcpy(allHeaders[numHeaders].key, "Content-Length");
//    strcpy(allHeaders[numHeaders].value, response);
//    numHeaders++;
//
//    writeHeaders(socket, statusCode, allHeaders, numHeaders);
//
//    write(socket, content, strlen(content));
//    return 0;
//}
//
//void writeHeaders(int socket, int statusCode, Header **headers, int numHeaders)
//{
//    char header[numHeaders * MAX_HEADER_LEN + 50];
//    int i;
//
//    sprintf(header, "HTTP/1.1 %d %s\r\n", statusCode, getStatusCodeName(statusCode));
//    for (i = 0; i < numHeaders; i++) {
//        sprintf(header[strlen(header)], "%s: %s\r\n", headers[i]->key, headers[i]->value);
//    }
//    sprintf(header[strlen(header)], "Date: Sat, 23 Jan 2016 04:15:54 GMT\r\nServer: CS360L2\r\n\r\n");
//
//    write(socket, header, strlen(header));
//}

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