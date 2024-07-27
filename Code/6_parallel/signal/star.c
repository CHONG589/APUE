#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>

static void int_handler(int s) {
    write(1, "!", 1);
}

int main () {

    int i;

    //signal(SIGINT, SIG_IGN);
    signal(SIGINT, int_handler);

    for (i = 0; i < 10; i++) {
        //ssize_t write(int fd, const void *buf, size_t count);
        // fd = 1 ，1 表示标准输出，即终端，count = 1 表示一个字节
        write(1, "*", 1);
        sleep(1);
    }
    printf("\n");

    exit(0);
}
