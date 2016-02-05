#include <stdlib.h>
#include <pthread.h>
#include "threads.h"
#include "server.h"

ThreadPool **threadPool;

void initializeThreads(int numThreads)
{
    int i;
    pthread_t threads[numThreads];
    threadPool = malloc(sizeof(ThreadPool) * numThreads);
    for (i = 0; i < numThreads; i++) {
        threadPool[i]->threadId = pthread_create(&threads[i], NULL, handleRequest, NULL);
        threadPool[i]->busy = 0;
    }
}

int enQueue(int socket)
{
    // TODO lock the thread pool semaphores and find one that isn't busy
    // Keep searching until a free thread is found, provide max loop timeout
}