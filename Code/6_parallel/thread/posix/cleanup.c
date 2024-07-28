#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <unistd.h>

static void cleanup_func(void *p) {
    puts(p);
}

static void *func(void *p) {
    puts("Thread is working!");

    // 这样写会出错，因为这个函数是宏，查看它是不是宏可以查看预处理后的代码。
    // gcc cleanup.c -E
    pthread_cleanup_push(cleanup_func, "cleanup:1");
    pthread_cleanup_push(cleanup_func, "cleanup:2");
    pthread_cleanup_push(cleanup_func, "cleanup:3");

    pthread_cleanup_pop(1);
    pthread_cleanup_pop(1);
    pthread_cleanup_pop(1);

    pthread_exit(NULL);
}

int main() {

    pthread_t tid;
    int err;

    puts("Begin!");

    err = pthread_create(&tid, NULL, func, NULL);
    if (err) {
        fprintf(stderr, "pthread_create(): %s\n", strerror(err));
        exit(1);
    }

    pthread_join(tid, NULL);

    puts("End!");

    exit(0);
}
