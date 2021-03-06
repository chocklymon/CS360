#ifndef LAB3_HTTP_H
#define LAB3_HTTP_H

#define MAX_HEADER_LEN            1024
// Define the max line length as the double the max header length + 2
// a header line is [key]: [value] The + 2 covers the ': ' character sequence.
#define MAX_LINE_LENGTH           (MAX_HEADER_LEN * 2) + 2

#define HTTP_OK                    200
#define HTTP_BAD_REQUEST           400
#define HTTP_NOT_FOUND             404
#define HTTP_METHOD_NOT_ALLOWED    405
#define HTTP_INTERNAL_SERVER_ERROR 500
#define HTTP_NOT_IMPLEMENTED       501
#define HTTP_VERSION_NOT_SUPPORTED 505

#define HEADER_ERROR               -3

typedef struct header
{
    char* key;
    char* value;
} Header;

Header *createHeader(const char *key, const char *value);
Header *createHeaderInt(const char *key, int value);
char *getStatusCodeName(int code);
void freeHeaders(Header **headers, int numHeaders);
int readHeaders(int socket, Header headers[], int *numHeaders, int maxNumHeaders);

#endif //LAB3_HTTP_H
