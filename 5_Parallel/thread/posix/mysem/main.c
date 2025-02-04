#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

#include "mysem.h"

#define LEFT        30000000
#define RIGHT       30000200
#define THRNUM      (RIGHT-LEFT+1)
#define N           4

/**
 * 筛选指定范围的质:
 */
static mysem_t *sem;

static void *thr_primer(void *p) {

    int i, j, mark;
    i = (int)p;
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

    sleep(5);
    mysem_add(sem, 1);
    pthread_exit(NULL);
}

int main() {

    /**
     * 下面这段程序是以前的写法，相当于一个进程在做的事情。
     */
    int i, err;
    pthread_t tid[THRNUM];

    sem = mysem_init(N);
    if (sem == NULL) {
        fprintf(stderr, "mysem_init() failed!\n");
        exit(1);
    }

    for (i = LEFT; i <= RIGHT; i++) {
        mysem_sub(sem, 1);
        err = pthread_create(tid + (i - LEFT), NULL, thr_primer, (void *)i);
        if (err) {
            fprintf(stderr, "pthread_create(): %s\n", strerror(err));
            exit(1);
        }
    } 
    for (i = LEFT; i <= RIGHT; i++) {
        pthread_join(tid[i - LEFT], NULL);
    }

    mysem_destroy(sem);

    exit(0);
}
