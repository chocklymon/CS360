#ifndef LAB2_SERVER_H
#define LAB2_SERVER_H

#define SOCKET_ERROR        -1
#define QUEUE_SIZE          5
#define MAX_NUM_HEADERS     50

int getLine(int socket, char *buffer, int i);

#endif //LAB2_SERVER_H
