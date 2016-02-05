#ifndef LAB3_SERVER_H
#define LAB3_SERVER_H

#define SOCKET_ERROR        -1
#define QUEUE_SIZE          5
#define MAX_NUM_HEADERS     50

int getLine(int socket, char *buffer, int i);
void *socketHandler(void *arg);

#endif //LAB3_SERVER_H
