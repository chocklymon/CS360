#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv)
{
    int port;
    char *directory;

    // Command Line Arguments //
    if (argc < 3) {
        printf("usage: %s <port> <dir>\n", argv[0]);
        return 1;
    }
    port = atoi(argv[1]);
    directory = argv[2];

    if (port <= 0) {
        printf("Invalid port provided.\n");
        return 1;
    }

    // TODO server

    return 0;
}