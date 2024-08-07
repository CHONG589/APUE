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
static pthread_cond_t mut_cond = PTHREAD_COND_INITIALIZER;

static void *thr_primer(void *p) {

    int i, j, mark;

    while (1) {

        pthread_mutex_lock(&mut_num);
        while (num == 0) {
            pthread_cond_wait(&mut_cond, &mut_num);
        }

        if (num == -1) {
            // 不能直接 break 因为还没有解锁，其它线程会被阻塞
            // 在 mut_num 中。
            pthread_mutex_unlock(&mut_num);
            break;
        }
        i = num;
        num = 0;
        pthread_cond_broadcast(&mut_cond);
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
            pthread_cond_wait(&mut_cond, &mut_num);
        }
        num = i;
        pthread_cond_signal(&mut_cond);
        pthread_mutex_unlock(&mut_num);
    }

    pthread_mutex_lock(&mut_num);
    while (num != 0) {
        pthread_cond_wait(&mut_cond, &mut_num);
    }
    num = -1;
    pthread_cond_broadcast(&mut_cond);
    pthread_mutex_unlock(&mut_num);

    for (i = 0; i <= THRNUM; i++) {
        pthread_join(tid[i], NULL);
    }

    pthread_mutex_destroy(&mut_num);
    pthread_cond_destroy(&mut_cond);

    exit(0);
}
