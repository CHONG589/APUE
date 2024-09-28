#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <pthread.h>
#include <sched.h>

#define LEFT        30000000
#define RIGHT       30000200
#define THRNUM      4

/**
 * 筛选指定范围的质数
 */

static int num = 0;
static pthread_mutex_t mut_num = PTHREAD_MUTEX_INITIALIZER;

static void *thr_primer(void *p) {

    int i, j, mark;

    while (1) {

        pthread_mutex_lock(&mut_num);
        while (num == 0) {
            // 如果为 0，解锁让 main 有机会分配
            pthread_mutex_unlock(&mut_num);
            sched_yield();
            pthread_mutex_lock(&mut_num);
        }

        if (num == -1) {
            // 不能直接 break 因为还没有解锁，其它线程会被阻塞
            // 在 mut_num 中。
            pthread_mutex_unlock(&mut_num);
            break;
        }
        i = num;
        num = 0;
        pthread_mutex_unlock(&mut_num);

        mark = 1;
        for (j = 2; j < i / 2; j++) {
            if (i % j == 0) {
                mark = 0;
                break;
            }
        }
        if (mark) {
            printf("[%d]%d is a primer\n", (int)p, i);
        }
    }

    pthread_exit(NULL);
}

int main() {

    /**
     * 下面这段程序是以前的写法，相当于一个进程在做的事情。
     */
    int i, err;
    pthread_t tid[THRNUM];

    for (i = 0; i <= THRNUM; i++) {
        err = pthread_create(tid + i, NULL, thr_primer, (void *)i);
        if (err) {
            fprintf(stderr, "pthread_create(): %s\n", strerror(err));
            exit(1);
        }
    } 

    for (i = LEFT; i <= RIGHT; i++) {
        pthread_mutex_lock(&mut_num);
        while (num != 0) {
            pthread_mutex_unlock(&mut_num);
            // 出让调度器给别的线程使用，而不是像 sleep 一样让
            // 线程从 running 变为 sleep 状态，从 running ->
            // sleep -> running 是调度状态的颠簸，没有必要发生。
            sched_yield();
            pthread_mutex_lock(&mut_num);
        }
        num = i;
        pthread_mutex_unlock(&mut_num);
    }

    pthread_mutex_lock(&mut_num);
    while (num != 0) {
        pthread_mutex_unlock(&mut_num);
        sched_yield();
        pthread_mutex_lock(&mut_num);
    }
    num = -1;
    pthread_mutex_unlock(&mut_num);

    for (i = 0; i <= THRNUM; i++) {
        pthread_join(tid[i], NULL);
    }

    pthread_mutex_destroy(&mut_num);

    exit(0);
}
