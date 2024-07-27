#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

#define LEFT    30000000
#define RIGHT   30000200

/**
 * 筛选指定范围的质数
 */

int main() {

    /**
     * 用 201 各子进程来代替这 201 个计算任务，即每产生一个
     * i 值就扔给 fork 进行计算。
     */
    int i, j, mark, pid;

    for(i = LEFT; i <= RIGHT; i++) {
        pid = fork();
        if (pid < 0){
            perror("fork()");
            exit(1);
        }
        if (pid == 0) {
            // child
            mark = 1;
            for (j = 2; j < i / 2; j++) {
                if (i % j == 0) {
                    mark = 0;
                    break;
                }
            }
            if (mark) {
                printf("%d is a primer\n", i);
            }
            exit(0);
        }
    }

    // 收尸
    int st;
    for (i = LEFT; i <= RIGHT; i++) {
        wait(NULL);
    }

    exit(0);
}
