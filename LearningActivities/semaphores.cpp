#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <queue>

#define NUM_THREADS 10
#define NQUEUE 20
#define MAX_LOOP 10

sem_t empty, full, mutex;
void *printThreadId(void *threadid);

class socketqueue {
    std::queue<int> queue;
public:
    void push(int socket)
    {
        sem_wait(&empty);
        sem_wait(&mutex);
        queue.push(socket);
        sem_post(&mutex);
        sem_post(&full);
    }

    int pop()
    {
        sem_wait(&full);
        sem_wait(&mutex);
        int val = queue.front();
        queue.pop();
        sem_post(&mutex);
        sem_post(&empty);
        return val;
    };
} SockQueue;

int main (int argc, char *argv[])
{
    pthread_t threads[NUM_THREADS];
    int rc, threadId, i;

    sem_init(&empty, PTHREAD_PROCESS_PRIVATE, NQUEUE);
    sem_init(&full, PTHREAD_PROCESS_PRIVATE, 0);
    sem_init(&mutex, PTHREAD_PROCESS_PRIVATE, 1);

//    for (i = 0; i < NQUEUE; i++) {
//        SockQueue.push(i);
//    }
//    for (i = 0; i < NQUEUE; i++) {
//        printf("%02d) GOT %d\n", i, SockQueue.pop());
//    }

    for (threadId = 0; threadId < NUM_THREADS; threadId++) {
        rc = pthread_create(&threads[threadId], NULL, printThreadId, (void *) threadId);
        if (rc) {
            printf("ERROR; return code from pthread_create() is %d\n", rc);
            exit(-1);
        }
    }

    for (i = 0; i < MAX_LOOP * NUM_THREADS; i++) {
        SockQueue.push(i);
    }

    pthread_exit(NULL);
}

void *printThreadId(void *arg)
{
    // Handling Sockets
    int i;
    long tid;
    tid = (long) arg;
    for (i = 0; i < MAX_LOOP; i++) {
        printf("Thread %02ld - (%02d) Socket: %d\n", tid, i, SockQueue.pop());
        sleep(1);
    }
    pthread_exit(NULL);
}