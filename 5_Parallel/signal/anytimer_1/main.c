#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#include "anytimer.h"

/**
 * 使用单一计时器实现多任务计时器
 */

static void f2(char *p) {
    printf("f2(): %s\n", p);
}

static void f1(char *p) {
    printf("f1(): %s\n", p);
}

int main() {

    // 输出效果：Begin!End!..bbb...aaa..ccc.......

    int job1, job2, job3;
    
    puts("Begin!");
    at_addjob_repeat(2, f1, "aaa");
    if (job1 < 0) {
        fprintf(stderr, "at_addjob(): %s\n", strerror(-job1));
        exit(1);
    }

#if 0
    at_addjob(2, f2, "bbb");
    at_addjob(7, f1, "ccc");
    puts("End!");
#endif

    while (1) {
        write(1, ".", 1);
        sleep(1);
    }

    exit(0);
}
