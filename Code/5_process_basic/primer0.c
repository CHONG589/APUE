#include <stdio.h>
#include <stdlib.h>

#define LEFT    30000000
#define RIGHT   30000200

/**
 * 筛选指定范围的质数
 */

int main() {

    /**
     * 下面这段程序是以前的写法，相当于一个进程在做的事情。
     */
    int i, j, mark;

    for(i = LEFT; i <= RIGHT; i++) {
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
    }
    // real    0m0.664s
    // user    0m0.658s
    // sys     0m0.005s

    exit(0);
}
