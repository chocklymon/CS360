#include <stdlib.h>
#include <string.h>
#include "http.h"
#include "server.h"
#include "utils.h"

Header *createHeader(char *key, char *value)
{
    Header *header = malloc(sizeof(Header));
    header->key = malloc(strlen(key));
    strcpy(header->key, key);
    header->value = malloc(strlen(value));
    strcpy(header->value, value);

    return header;
}

Header *createHeaderInt(char *key, int value)
{
    char strValue[15];
    sprintf(strValue, "%d", value);
    return createHeader(key, strValue);
}

char *getStatusCodeName(int code)
{
    switch (code) {
        case HTTP_OK:
            return "OK";
        case HTTP_BAD_REQUEST:
            return "Bad Request";
        case HTTP_NOT_FOUND:
            return "Not Found";
        case HTTP_METHOD_NOT_ALLOWED:
            return "Method Not Allowed";
        case HTTP_INTERNAL_SERVER_ERROR:
            return "Internal Server Error";
        case HTTP_NOT_IMPLEMENTED:
            return "Not Implemented";
    }
    // Code wasn't recognized
    return "";
}


void freeHeaders(Header **headers, int numHeaders)
{
    int i;
    for (i = 0; i < numHeaders; i++) {
        if (headers[i]) {
            free(headers[i]->key);
            free(headers[i]->value);
            free(headers[i]);
            headers[i] = NULL;
        }
    }
}

int readHeaders(int socket, Header **headers, int *numHeaders, int maxNumHeaders)
{
    char line[MAX_LINE_LENGTH + 1];
    char *loc;
    Header *header;
    *numHeaders = 0;

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
                return HEADER_ERROR;
            }
            header = malloc(sizeof(Header));
            header->key = malloc(strlen(line));
            strcpy(header->key, line);

            loc++;

            loc = trimLeft(loc);
            if (strlen(loc) > MAX_HEADER_LEN) {
                return HEADER_ERROR;
            }
            header->value = malloc(strlen(loc));
            strcpy(header->value, loc);

            headers[*numHeaders] = header;
            (*numHeaders)++;

            if (*numHeaders >= maxNumHeaders) {
                printf("Too many headers received.\n");
                return HEADER_ERROR;
            }
        }

        if (getLine(socket, line, MAX_LINE_LENGTH) == SOCKET_ERROR) {
            return SOCKET_ERROR;
        }
    }

    return 0;
}