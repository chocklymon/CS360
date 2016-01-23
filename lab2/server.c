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
#include <arpa/inet.h>
#include "server.h"
#include "http.h"
#include "utils.h"

void writeError(int socket, int statusCode);
void writeHeaders(int socket, int statusCode, Header **headers, int numHeaders);

void writeFileResponse(int socket, const char *file, struct stat fileStat);

void writeBasicResponse(int socket, int statusCode, const char *content, const char *contentType, int contentLength);

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
    char *method, *loc, *resource;
    char directory[MAX_LINE_LENGTH + 1];
    struct stat filestat;
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
    // Remove the trailing backslash if there is one
    if (webDirectory[strlen(webDirectory) - 1] == '/') {
        webDirectory[strlen(webDirectory) - 1] = 0;
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
            printf(" - Got a connection from %s:%d\n", inet_ntoa(Address.sin_addr), ntohs(Address.sin_port));
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

            // Parse the HTTP header //
            // Get the method
            method = pBuffer;
            loc = strchr(pBuffer, ' ');
            *loc = 0;
            loc++;

            // Get the requested resource
            resource = loc;
            loc = strchr(loc, ' ');
            *loc = 0;
            loc++;

            // Check the HTTP version
            if (strcmp(loc, "HTTP/1.1") == 0 || strcmp(loc, "HTTP/1.0") == 0) {
                printf("Received Request -- Method: %s Resource: %s\n", method, resource);
                // TODO

                strcpy(directory, webDirectory);
                strcat(directory, resource);

                if (stat(directory, &filestat)) {
                    if (verbose) {
                        printf(" - Unable to stat %s\n", directory);
                    }
                    writeError(hSocket, HTTP_NOT_FOUND);
                } else if (S_ISREG(filestat.st_mode)) {
                    if (verbose) {
                        printf(" - %s is a regular file.\n", directory);
                    }
                    writeFileResponse(hSocket, directory, filestat);
                } else if (S_ISDIR(filestat.st_mode)) {
                    // TODO
                    // Look for an index.html in the directory
                    //  if exists
                    //    serve index.html
                    //  else
                    //    create and serve a directory listing
                    printf("%s is a directory\n", directory);

                    // Using dirent to print all the files in the directory
                    DIR *dirp;
                    struct dirent *dp;

                    dirp = opendir(directory);
                    while ((dp = readdir(dirp)) != NULL) {
                        printf("name %s\n", dp->d_name);
                    }
                    closedir(dirp);

                    writeError(hSocket, HTTP_NOT_IMPLEMENTED);
                }
            } else {
                writeError(hSocket, HTTP_VERSION_NOT_SUPPORTED);
            }
        }

        // Clean up any headers
        freeHeaders(inputHeaders, numHeaders);
        numHeaders = 0;

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

void writeFileResponse(int socket, const char *file, struct stat fileStat)
{
    // Read the file
    FILE *fp = fopen(file, "r");
    char *buffer = malloc((fileStat.st_size + 1) * sizeof(char));
    if (fread(buffer, fileStat.st_mode, 1, fp)) {
        fclose(fp);
        perror("Problem reading file");
        writeError(socket, HTTP_INTERNAL_SERVER_ERROR);
    } else {
        fclose(fp);

        // Try to get the content type
        char *contentType;
        char *extension = getExtension(file);
        printf("Extension: %s\n", extension);
        if (strcmp(extension, "html") == 0) {
            contentType = "text/html";
        } else if (strcmp(extension, "css") == 0) {
            contentType = "text/css";
        } else if (strcmp(extension, "js") == 0) {
            contentType = "application/javascript";
        } else if (strcmp(extension, "jpg") == 0 || strcmp(extension, "jpeg") == 0) {
            contentType = "image/jpg";
        } else if (strcmp(extension, "gif") == 0) {
            contentType = "image/gif";
        } else if (strcmp(extension, "png") == 0) {
            contentType = "image/png";
        } else if (strcmp(extension, "ico") == 0) {
            contentType = "image/x-icon";
        } else {
            // Default case
            contentType = "text/plain";
        }

        // Send the file
        writeBasicResponse(socket, HTTP_OK, buffer, contentType, (int) fileStat.st_size);
    }
    free(buffer);
}

void writeBasicResponse(int socket, int statusCode, const char *content, const char *contentType, int contentLength)
{
    // Prepare and send the headers
    Header **headers = malloc(3 * sizeof(Header*));
    headers[0] = createHeader("Connection", "Close");
    headers[1] = createHeader("Content-Type", contentType);
    headers[2] = createHeaderInt("Content-Length", contentLength);
    writeHeaders(socket, statusCode, headers, 3);

    // Send the content
    write(socket, content, (size_t) contentLength);

    // Clean Up
    freeHeaders(headers, 3);
    free(headers);
}

void writeError(int socket, int statusCode)
{
    // Generate an error response body
    char responseBody[MAX_LINE_LENGTH];
    char* statusCodeName = getStatusCodeName(statusCode);
    sprintf(
        responseBody,
        "<!DOCTYPE html>\n<html lang=\"en\">\n<head><title>Error - %s</title></head>\n<body><h1>Error %d</h1><p>%s</p></body>\n</html>",
        statusCodeName,
        statusCode,
        statusCodeName
    );

    writeBasicResponse(socket, statusCode, responseBody, "text/html", (int) strlen(responseBody));
}

void writeHeaders(int socket, int statusCode, Header **headers, int numHeaders)
{
    char header[numHeaders * MAX_HEADER_LEN + 50];
    int i;

    sprintf(header, "HTTP/1.1 %d %s\r\n", statusCode, getStatusCodeName(statusCode));
    for (i = 0; i < numHeaders; i++) {
        sprintf(&header[strlen(header)], "%s: %s\r\n", headers[i]->key, headers[i]->value);
    }

    // Add the standard headers
    char* date = getDate();
    sprintf(&header[strlen(header)], "Date: %s\r\nServer: CS360L2\r\n\r\n", date);
    free(date);

    write(socket, header, strlen(header));
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
