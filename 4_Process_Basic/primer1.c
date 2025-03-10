#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define LEFT    30000000
#define RIGHT   30000200

/**
 * 筛选指定范围的质数
 */

int main() {

    /**
     * 用 201 个子进程来代替这 201 个计算任务，即每产生一个
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
            //sleep(1000);
            exit(0);
        }
    }
    // real    0m0.039s
    // user    0m0.002s
    // sys     0m0.024s

    sleep(1000);
    exit(0);
}
