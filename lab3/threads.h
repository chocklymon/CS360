#ifndef LAB3_THREADS_H
#define LAB3_THREADS_H

#define THREAD_QUEUE_ERROR -1

typedef struct theadpool
{
    int threadId;
    int busy;
} ThreadPool;

void initializeThreads(int numThreads);
int enQueue(int socket);

#endif //LAB3_THREADS_H
