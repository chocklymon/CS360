#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

#define NUM_THREADS 20

void *printThreadId(void *threadid);

int main (int argc, char *argv[])
{
    pthread_t threads[NUM_THREADS];
    int rc, threadId;
    for (threadId = 0; threadId < NUM_THREADS; threadId++) {
        rc = pthread_create(&threads[threadId], NULL, printThreadId, (void *) threadId);
        if (rc) {
            printf("ERROR; return code from pthread_create() is %d\n", rc);
            exit(-1);
        }
    }

    // TODO for server
    // Accept, the connection
    // enQueue(socket)

    pthread_exit(NULL);
}

void enQueue(int socket)
{
    // TODO add the socket to the queue
    // signal semaphore
}

int deQueue()
{
    // TODO wait on semaphore
    // return socket number from the queue
    return -1;
}

void *printThreadId(void *arg)
{
    // Handling Sockets
    sock = deQueue();
    long tid;
    tid = (long) arg;
    printf("Hi %ld\n", tid);
}