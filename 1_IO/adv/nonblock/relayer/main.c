#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

#include "relayer.h"

#define TTY1        "/dev/tty11"
#define TTY2        "/dev/tty12"
#define TTY3        "/dev/tty10"
#define TTY4        "/dev/tty9"

int main() {

    int fd1, fd2, fd3, fd4;
    int job1, job2;

    // 阻塞打开
    fd1 = open(TTY1, O_RDWR);
    if (fd1 < 0) {
        perror("open()");
        exit(1);
    }
    write(fd1, "TTY1\n", 5);

    // 非阻塞打开
    fd2 = open(TTY2, O_RDWR | O_NONBLOCK);
    if (fd2 < 0) {
        perror("open()");
        exit(1);
    }
    write(fd2, "TTY2\n", 5);

    job1 = rel_addjob(fd1, fd2);
    if (job1 < 0) {
        fprintf(stderr, "rel_addjob(): %s\n", strerror(-job1));
        exit(1);
    }

    fd3 = open(TTY3, O_RDWR);
    if (fd3 < 0) {
        perror("open()");
        exit(1);
    }
    write(fd3, "TTY3\n", 5);

    // 非阻塞打开
    fd4 = open(TTY4, O_RDWR | O_NONBLOCK);
    if (fd4 < 0) {
        perror("open()");
        exit(1);
    }
    write(fd4, "TTY4\n", 5);

    job2 = rel_addjob(fd3, fd4);
    if (job2 < 0) {
        fprintf(stderr, "rel_addjob(): %s\n", strerror(-job2));
        exit(1);
    }

    while(1) {
        pause();
    }

    close(fd4);
    close(fd3);
    close(fd2);
    close(fd1);

    exit(0);
}
