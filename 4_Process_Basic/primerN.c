#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

#define LEFT    30000000
#define RIGHT   30000200
#define N       3

/**
 * 筛选指定范围的质数
 */

int main() {

    /**
     * 用 201 各子进程来代替这 201 个计算任务，即每产生一个
     * i 值就扔给 fork 进行计算。
     */
    int i, j, mark, n;
    pid_t pid;

    for (n = 0; n < N; n++) {
        pid = fork();
        if (pid < 0){
            // 如果中间某个进程创建失败，那么前面创建的进程
            // 资源要释放掉。
            perror("fork()");
            exit(1);
        }
        if (pid == 0) {
            for(i = LEFT + n; i <= RIGHT; i += N) {
                
                // child
                mark = 1;
                for (j = 2; j < i / 2; j++) {
                    if (i % j == 0) {
                        mark = 0;
                        break;
                    }
                }
                if (mark) {
                    printf("[%d]: %d is a primer\n", n, i);
                }
                //exit(0);
            }
            // 将全部数值处理完后再退出。
            exit(0);
        }
    }

    // 收尸
    int st;
    for (n = 0; n < N; n++) {
        wait(NULL);
    }

    exit(0);
}
