#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>

#define MEMSIZE     1024

int main() {

    char *ptr;
    pid_t pid;

    // 这里不是从哪个文件中映射过来，即匿名映射，不依赖于
    // 任何文件，所以参数中的 fd 为 -1。

    // 这里约定为子进程写，父进程进行读。
    ptr = mmap(NULL, MEMSIZE, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    if (ptr == MAP_FAILED) {
        perror("mmap()");
        exit(1);
    }

    pid = fork();
        if (pid < 0) {
        perror("fork()");
        munmap(ptr, MEMSIZE);
        exit(1);
    }
    if (pid == 0) {
        // child write 
        strcpy(ptr, "Hello! Ling Mei");
        munmap(ptr, MEMSIZE);
        exit(0);
    }
    else {
        // parent read
        wait(NULL);
        puts(ptr);
        munmap(ptr, MEMSIZE);
        exit(0);
    }
}
