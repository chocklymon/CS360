
#define MAX_HEADER_LEN          1024

#define HTTP_OK                  200
#define HTTP_BADREQUEST          400
#define HTTP_NOTFOUND            404
#define HTTP_INTERNALSERVERERROR 500

struct header {
    char key[MAX_HEADER_LEN];
    char value[MAX_HEADER_LEN];
};