#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/signal.h>

void handler (int status);   /* definition of signal handler */
int  counter = 0;

int main(int argc, char **argv)
{
    int rc1, rc2;

    // First set up the signal handler
    struct sigaction sigold, signew;

    signew.sa_handler = handler;
    sigemptyset(&signew.sa_mask);
    sigaddset(&signew.sa_mask, SIGINT);
    signew.sa_flags = SA_RESTART;
    sigaction(SIGINT, &signew, &sigold);
    sigaction(SIGHUP, &signew, &sigold);
    sigaction(SIGPIPE, &signew, &sigold);

    for(; counter < 5;);

    return 0;
}

void handler (int status)
{
    printf("received signal %d (Count: %d)\n", status, counter);
    counter++;
}