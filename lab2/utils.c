#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "utils.h"

char *getDate()
{
    time_t t = time(NULL);
    struct tm tm = *gmtime(&t);

    // Sat, 23 Jan 2016 04:15:54 GMT
    char *date = malloc(150 * sizeof(char));
    strftime(date, 150, "%a, %d %b %Y %H:%M:%S %Z", &tm);
    return date;
}

char *getExtension(const char *fileName)
{
    char *loc = strrchr(fileName, '/');
    if (loc == 0) {
        loc = strrchr(fileName, '.');
    } else {
        loc = strrchr(loc, '.');
    }
    if (loc) {
        loc++;
        return loc;
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
