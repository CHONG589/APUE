#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <unistd.h>

#define MEMSIZE     1024

int main() {

    char *ptr;
    pid_t pid;
    int shmid;

    shmid = shmget(IPC_PRIVATE, MEMSIZE, 0600);
    if (shmid < 0) {
        perror("shmget()");
        exit(1);
    }

    pid = fork();
        if (pid < 0) {
        perror("fork()");
        exit(1);
    }
    if (pid == 0) {
        // child write 
        // 第二个参数是指映射到的地址空间，写 NULL 则
        // 表示系统自动分配，第三个为特殊要求，如要求
        // 那片空间只读等。
        ptr = shmat(shmid, NULL, 0);
        if (ptr == (void *)-1) {
            perror("shmat()");
            exit(1);
        }
        strcpy(ptr, "Hello!");
        shmdt(ptr);   // 销毁空间
        exit(0);
    }
    else {
        // parent read
        // 将创建的共享内存映射给父进程。

        wait(NULL);// 先收尸，确保子进程已经写进去
        ptr = shmat(shmid, NULL, 0);
        if (ptr == (void *)-1) {
            perror("shmat()");
            exit(1);
        }
        puts(ptr);
        shmdt(ptr);
        shmctl(shmid, IPC_RMID, NULL);
        exit(0);
    }
}
