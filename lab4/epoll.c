//
// Fake EPOLL for testing on Mac OS X
//

#include <stdlib.h>
#include "epoll.h"

int *fdList;
int size;

extern int epoll_create(int __size)
{
    fdList = malloc(__size * sizeof(int));
    size = 0;
    return 1;
}


extern int epoll_ctl (int __epfd, int __op, int __fd,
                      struct epoll_event *__event)
{
    fdList[size++] = __fd;
    return 1;
}

extern int epoll_wait (int __epfd, struct epoll_event *__events,
                       int __maxevents, int __timeout)
{
    if (size > 0) {
        __events->data.fd = fdList[size--];
    }
    return 0;
}
