#ifndef LAB3_UTILS_H
#define LAB3_UTILS_H

char *getExtension(const char *fileName);
char *getDate();
int isWhiteSpace(const char c);
void stripTrailingSlash(char *str);
char *substringTo(char *dest, const char *source, int divider);
char *trimLeft(char *str);
void trimRight(char *str);

#endif //LAB3_UTILS_H
