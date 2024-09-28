#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>

#define LEFT        30000000
#define RIGHT       30000200
#define THRNUM      (RIGHT-LEFT+1)

/**
 * 筛选指定范围的质数
 */

struct thr_arg_st {
    int n;
};

static void *thr_primer(void *p) {

    int i, j, mark;
    // 先将 void *p 强转成 struct thr_arg_st *，所以是
    // 先将 p 强转，然后再取值。
    i = ((struct thr_arg_st *)p)->n;
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
    // 将 p 返回去，在 main 中使用 pthread_join 的第二个
    // 参数可以接收这个 p。
    pthread_exit(p);
}

int main() {

    /**
     * 下面这段程序是以前的写法，相当于一个进程在做的事情。
     */
    int i, err;
    pthread_t tid[THRNUM];
    struct thr_arg_st *p;
    void *ptr;

    for (i = LEFT; i <= RIGHT; i++) {
        p = malloc(sizeof(*p));
        if (p == NULL) {
            perror("malloc()");
            exit(1);
        }
        p->n = i;
        err = pthread_create(tid + (i - LEFT), NULL, thr_primer, p);
        if (err) {
            fprintf(stderr, "pthread_create(): %s\n", strerror(err));
            exit(1);
        }
    } 
    for (i = LEFT; i <= RIGHT; i++) {
        // 第二个参数接收线程的返回值。
        pthread_join(tid[i - LEFT], &ptr);
        free(ptr);
    }

    exit(0);
}
