#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <unistd.h>
#include <netdb.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <semaphore.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include "server.h"
#include "http.h"
#include "utils.h"

void handler(int status);
int runServer(int hServerSocket, struct sockaddr_in Address, int nAddressSize, const char *webDirectory, int verbose);
void serveGetRequest(int socket, const char *directory, char *resource, int verbose);
void writeBasicResponse(int socket, int statusCode, const char *content, const char *contentType, int contentLength);
void writeError(int socket, int statusCode);
void writeFileResponse(int socket, const char *file, struct stat fileStat);
void writeHeaders(int socket, int statusCode, Header **headers, int numHeaders);
int handleRequest(int hSocket, const char *webDirectory, int verbose);

sem_t queueMutex;
sem_t socketThreadsSem;
int socketQueue[MAX_QUEUE_SIZE + 1];

int main(int argc, char **argv)
{
    int port, numThreads;
    int nAddressSize = sizeof(struct sockaddr_in);
    int opt, runResult;
    int verbose = 0;
    int hServerSocket;  // Handle to socket
    char *webDirectory;
    struct sockaddr_in Address; // Internet socket address stuct

    // Command Line Arguments //
    if (argc < 4) {
        printf("usage: %s [-v] <port> <numThreads> <dir>\n", argv[0]);
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
    numThreads = atoi(argv[optind + 1]);
    webDirectory = argv[optind + 2];

    if (port <= 0) {
        printf("Invalid port provided.\n");
        return 1;
    }
    if (numThreads <= 0) {
        printf("Invalid number of threads provided.\n");
        return 1;
    }
    // Remove the trailing backslash if there is one
    stripTrailingSlash(webDirectory);

    // Set up the signal handlers
    struct sigaction sigold, signew;

    signew.sa_handler = handler;
    sigemptyset(&signew.sa_mask);
    sigaddset(&signew.sa_mask, SIGINT);
    signew.sa_flags = SA_RESTART;
    sigaction(SIGHUP, &signew, &sigold);
    sigaction(SIGPIPE, &signew, &sigold);

    // Semaphores and thread pool
    queue[0] = 0;
    sem_init(&queueMutex, 0, 1);
    sem_init(&socketThreadsSem, 0, 0);
    // TODO create the threads

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

    // Make the port available to be reused quicker after shutdown
    int optval = 1;
    setsockopt(hServerSocket, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));

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

    // Run the server
    printf("Server Running\n");
    runResult = runServer(hServerSocket, Address, nAddressSize, webDirectory, verbose);
    return runResult;
}

void handler(int status)
{
    printf("Received signal %d\n", status);
}

int enqueue(int *queue, int num)
{
    if (queue[0] >= MAX_QUEUE_SIZE) {
        return -1;
    } else {
        queue[++queue[0]] = num;
        return 0;
    }
}

int dequeue(int *queue)
{
    if (queue[0] == 0) {
        return -1;
    } else {
        return queue[queue[0]--];
    }
}

void addSocket(int socket)
{
    sem_wait(&queueMutex);
    enqueue(socketQueue, socket);
    sem_post(&queueMutex);

    // Wake up a connection handler thread
    sem_post(&socketThreadsSem);
}

int getSocket()
{
    int socketNum;
    sem_wait(&queueMutex);
    socketNum = dequeue(socketQueue);
    sem_signal(&queueMutex);
    return socketNum;
}

int runServer(int hServerSocket, struct sockaddr_in Address, int nAddressSize, const char *webDirectory, int verbose)
{
    int hSocket; // Client socket handle

    for (;;) {
        // Listen on the socket
        if (verbose) {
            printf(" - Waiting for a connection.\n");
        }
        hSocket = accept(hServerSocket, (struct sockaddr*) &Address, (socklen_t *) &nAddressSize);
        if (verbose) {
            printf(" - Got a connection from %s:%d\n", inet_ntoa(Address.sin_addr), ntohs(Address.sin_port));
        }

        addSocket(hSocket);
    }

    return 0;
}

int socketHandler()
{
    int hSocket;

    for (;;) {
        sem_wait(&socketThreadsSem);
        hSocket = getSocket();

        if (handleRequest(hSocket, webDirectory, verbose) == SOCKET_ERROR) {
            printf("Error found while handling a request. Shutting down thread.\n");
            return 2;
        }
    }
}

int handleRequest(int hSocket, const char *webDirectory, int verbose)
{
    int result, i;
    int numHeaders = 0;
    char httpHeader[MAX_LINE_LENGTH + 1];
    char *method, *loc, *resource;
    Header **inputHeaders = malloc(MAX_NUM_HEADERS * sizeof(Header*));

    // Read in the response
    if (getLine(hSocket, httpHeader, MAX_LINE_LENGTH) == SOCKET_ERROR) {
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
            printf("HTTP Headers Received:\n%s\n", httpHeader);
            for (i = 0; i < numHeaders; i++) {
                printf("%s: %s\n", inputHeaders[i]->key, inputHeaders[i]->value);
            }
        }

        // Parse the HTTP header //
        // Get the method
        method = httpHeader;
        loc = strchr(httpHeader, ' ');
        *loc = 0;
        loc++;

        // Get the requested resource
        resource = loc;
        loc = strchr(loc, ' ');
        *loc = 0;
        loc++;

        // Check the HTTP header
        // Method
        if (strcmp(method, "GET") != 0) {
            writeError(hSocket, HTTP_METHOD_NOT_ALLOWED);

            // Version
        } else if (strcmp(loc, "HTTP/1.1") == 0 || strcmp(loc, "HTTP/1.0") == 0) {
            // Handle the Request //
            printf("Received Request -- Method: %s Resource: %s\n", method, resource);
            serveGetRequest(hSocket, webDirectory, resource, verbose);
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
    struct linger lin;
    unsigned int y = sizeof(lin);
    lin.l_onoff=1;
    lin.l_linger=10;
    setsockopt(hSocket, SOL_SOCKET, SO_LINGER, &lin, sizeof(lin));
    shutdown(hSocket, SHUT_RDWR);
    if (close(hSocket) == SOCKET_ERROR) {
        perror("Failed to close the socket connection");
        return SOCKET_ERROR;
    }
    return 0;
}

void serveGetRequest(int hSocket, const char *webDirectory, char *resource, int verbose)
{
    char directory[MAX_LINE_LENGTH + 1];
    char dirFile[MAX_LINE_LENGTH + 1];
    struct stat filestat;
    DIR *dirp;
    struct dirent *dp;

    stripTrailingSlash(resource);

    // Get the full path to the file
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
        if (verbose) {
            printf(" - %s is a directory.\n", directory);
        }

        // Check for an index.html file
        dirFile[0] = 0;
        strcat(dirFile, directory);
        strcat(dirFile, "/index.html");
        if (stat(dirFile, &filestat)) {
            // No index.html file
            // Write out a directory listing
            if (verbose) {
                printf(" - Generating directory listing.\n");
            }
            char *directoryList = malloc((MAX_LINE_LENGTH * 2 + 1) * sizeof(char));
            sprintf(directoryList, "<!DOCTYPE html>\n<html lang=\"en\">\n<head><title>Directory Listing</title></head>\n<body>\n<h1>Index of %s</h1>\n<p><ul>\n", resource);

            // Using dirent to print all the files in the directory
            dirp = opendir(directory);
            while ((dp = readdir(dirp)) != NULL) {
                if (strcmp(dp->d_name, "..") == 0) {
                    if (resource[0] != 0) {
                        // Get the parent file
                        substringTo(dirFile, resource, '/');
                        sprintf(
                                &directoryList[strlen(directoryList)],
                                "<li><a href=\"%s\">Parent Directory</a></li>\n",
                                dirFile
                        );
                    }
                } else if (strcmp(dp->d_name, ".") != 0) {
                    sprintf(
                            &directoryList[strlen(directoryList)],
                            "<li><a href=\"%s/%s/\">%s</a></li>\n",
                            resource,
                            dp->d_name,
                            dp->d_name
                    );
                }
            }
            closedir(dirp);

            strcat(directoryList, "</ul></p></body>\n</html>");

            writeBasicResponse(hSocket, HTTP_OK, directoryList, "text/html", (int) strlen(directoryList));

            free(directoryList);
        } else {
            // Serve the index file
            if (verbose) {
                printf(" - Found index.html file in directory.\n");
            }
            writeFileResponse(hSocket, dirFile, filestat);
        }
    }
}

void writeFileResponse(int socket, const char *file, struct stat fileStat)
{
    // Read the file
    FILE *fp = fopen(file, "r");
    char *buffer = malloc((fileStat.st_size + 1) * sizeof(char));
    if (fread(buffer, fileStat.st_size, 1, fp) == -1) {
        fclose(fp);
        perror("Problem reading file");
        writeError(socket, HTTP_INTERNAL_SERVER_ERROR);
    } else {
        fclose(fp);

        // Try to get the content type
        char *contentType;
        char *extension = getExtension(file);
        if (extension) {
            if (strcmp(extension, "html") == 0) {
                contentType = "text/html";
            } else if (strcmp(extension, "xml") == 0) {
                contentType = "text/xml";
            } else if (strcmp(extension, "css") == 0) {
                contentType = "text/css";
            } else if (strcmp(extension, "js") == 0) {
                contentType = "application/javascript";
            } else if (strcmp(extension, "json") == 0) {
                contentType = "application/json";
            } else if (strcmp(extension, "jpg") == 0 || strcmp(extension, "jpeg") == 0) {
                contentType = "image/jpeg";
            } else if (strcmp(extension, "gif") == 0) {
                contentType = "image/gif";
            } else if (strcmp(extension, "png") == 0) {
                contentType = "image/png";
            } else if (strcmp(extension, "bmp") == 0) {
                contentType = "image/bmp";
            } else if (strcmp(extension, "tiff") == 0) {
                contentType = "image/tiff";
            } else if (strcmp(extension, "ico") == 0) {
                contentType = "image/x-icon";
            } else if (strcmp(extension, "svg") == 0) {
                contentType = "image/svg+xml";
            } else if (strcmp(extension, "woff") == 0) {
                contentType = "application/font-woff";
            } else if (strcmp(extension, "woff2") == 0) {
                contentType = "font/woff2";
            } else if (strcmp(extension, "ttf") == 0) {
                contentType = "application/x-font-ttf";
            } else if (strcmp(extension, "eot") == 0) {
                contentType = "application/vnd.ms-fontobject";
            } else {
                // Default case
                contentType = "text/plain";
            }
        } else {
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
    printf("Sending a HTTP %d error response.\n", statusCode);

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
        if (charactersRead == 0) {
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
