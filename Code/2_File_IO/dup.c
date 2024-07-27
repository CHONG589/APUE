#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#define FNAME   "./out"

int main() {

    // 每个进程中，都会默认打开 STDIN_FILENO、STDOUT_FILENO、STDERR_FILENO。
    // 它们三个的文件描述符分别代表 0, 1, 2。
    // 所以这里本来是默认用标准输出的，将它关掉，另外打开一个文件，将它的文件
    // 描述符放在 1 的位置。这样就输出到指定的文件中了。
    // int fd;
    // close(1);

    // fd = open(FNAME, O_WRONLY | O_CREAT | O_TRUNC, 0600);
    // if (fd < 0) {
    //     perror("open()");
    //     exit(1);
    // }

    int fd;
    fd = open(FNAME, O_WRONLY | O_CREAT | O_TRUNC, 0600);
    if (fd < 0) {
        perror("open()");
        exit(1);
    }

    dup2(fd, 1);
    if (fd != 1) {
        close(fd);
    }

    puts("hello!");

    exit(0);
}
