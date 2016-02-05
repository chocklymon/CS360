#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "utils.h"

char *getDate()
{
    time_t t = time(NULL);
    struct tm tm = *gmtime(&t);

    // Sat, 23 Jan 2016 04:15:54 GMT
    char *date = (char *) malloc(150 * sizeof(char));
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

void stripTrailingSlash(char *str)
{
    if (str[strlen(str) - 1] == '/') {
        str[strlen(str) - 1] = 0;
    }
}

char *substringTo(char *dest, const char *source, int divider)
{
    unsigned long index;
    char *to;
    to = strrchr(source, divider);

    if (to == NULL) {
        dest[0] = 0;
        return NULL;
    }

    index = (to + 1) - source;

    memcpy(dest, source, index);
    dest[index] = 0;

    return dest;
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
