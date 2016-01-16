#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

#define SOCKET_ERROR        -1
#define BUFFER_SIZE         100
#define HOST_NAME_SIZE      255

#define MAX_GET 1000

int main(int argc, char **argv)
{
    // Initialize variables
    int hSocket;                 // Handle to socket
    struct hostent* pHostInfo;   // Holds info about a machine
    struct sockaddr_in Address;  // Internet socket address stuct
    long nHostAddress;
    char pBuffer[BUFFER_SIZE];
    unsigned nReadAmount;
    char strHostName[HOST_NAME_SIZE];
    int nHostPort;
    int debug = 1;

    // Handle the command line arguments
    if (argc < 4) {
        printf("usage: download [-d] [-c COUNT] <host-name> <port> <file>\n");
        return 1;
    }

    strcpy(strHostName, argv[1]);
    nHostPort = atoi(argv[2]);

    // Make a socket
    hSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    if (hSocket == SOCKET_ERROR) {
        printf("Could not establish a connection\n");
        return 2;
    }

    // Get the IP address from name
    pHostInfo = gethostbyname(strHostName);
    /* copy address into long */
    memcpy(&nHostAddress, pHostInfo->h_addr, pHostInfo->h_length);

    /* fill address struct */
    Address.sin_addr.s_addr = nHostAddress;
    Address.sin_port = htons(nHostPort);
    Address.sin_family = AF_INET;

    if (debug) {
        printf("--Connecting to %s (%X) on port %d\n", strHostName, nHostAddress, nHostPort);
    }

    /* connect to host */
    if (connect(hSocket, (struct sockaddr*) &Address, sizeof(Address)) == SOCKET_ERROR) {
        printf("Could not connect to host\n");
        return 2;
    }

    char *message = malloc(MAX_GET);
    sprintf(message, "GET / HTTP/1.1\r\nHost: %s\r\nUser-agent: CBot\r\n\r\n", strHostName);
    // Send HTTP to the socket
    if (debug) {
        printf("Request:\n%s\n", message);
    }
    write(hSocket, message, strlen(message));
    // Read the response back from the socket
    nReadAmount = read(hSocket, pBuffer, BUFFER_SIZE);
    printf("Response:\n%s\n", pBuffer);

    if (close(hSocket) == SOCKET_ERROR) {
        printf("Could not close connection\n");
        return 2;
    }
}