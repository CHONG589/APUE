#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>

#include "mytbf.h"

typedef struct mytbf_st {
    int cps;
    int burst;
    int token;
    int pos;
} mytbf;

// typedef void (*sighandler_t)(int);

static mytbf *job[MYTBF_MAX];
static int inited = 0;
static __sighandler_t alrm_handler_save;

static void alrm_handler(int s) {
    int i;
    alarm(1);
    for (i = 0; i < MYTBF_MAX; i++) {
        // 只积累使用了的令牌桶，如果没使用的就步累加。
        if (job[i] != NULL) {
            // 每秒增加的速率，而这个速率是用户自己定义的，即 
            // cps，每过一秒就累加一次。
            job[i]->token += job[i]->cps;
            // 判断有没有超过自己定义的上限。
            if (job[i]->token > job[i]->burst)
                job[i]->token = job[i]->burst;
        }
    }
}

static void module_unload(void) {
    int i;
    // 恢复旧的信号行为函数。
    signal(SIGALRM, alrm_handler_save);
    alarm(0);// 终止定时

    // 释放空间。
    for (i = 0; i < MYTBF_MAX; i++)
        free(job[i]);
}

static void module_load(void) {
    alrm_handler_save = signal(SIGALRM, alrm_handler);
    alarm(1);
    atexit(module_unload);
}

static int get_free_pos(void) {
    int i;
    for (i = 0; i < MYTBF_MAX; i++) {
        if (job[i] == NULL) return i;
    }
    return -1;
}

mytbf_t *mytbf_init(int cps, int burst) {
    mytbf *me;
    int pos;

    if (!inited) {
        module_load();
        inited = 1;
    }

    // 找空位
    pos = get_free_pos();
    if (pos < 0) return NULL;

    me = malloc(sizeof(*me));
    if (me == NULL)
        return NULL;

    me->token = 0;  // 一定是从零开始
    me->cps = cps;
    me->burst = burst;
    me->pos = pos;

    job[pos] = me;

    return me;
}

static int min(int a, int b) {
    if (a < b) return a;
    return b;
}

// 取令牌，从第一个参数中取第二个参数的数量个令牌，将实际取得的数目
// 通过返回值返回来。
int mytbf_fetchtoken(mytbf_t *ptr, int size) {
    int n;
    mytbf *me = ptr;

    // 不能直接返回 EINVAL，因为它是一个正值，而这个函数要么返回
    // 取到的令牌个数，要么就是报错，如果你直接返回 EINVAL 正数，
    // 那么主函数中接收个数的值会误以为是返回的个数，而我们真正
    // 的目的是报错，所以这就造成了冲突，所以这时我们要返回 -EINVAL
    // 使它为负值，而主函数报错时再取反，这样就得到正确的报错。
    if (size <= 0) return -EINVAL;

    // 没有时要等待。
    while (me->token <= 0) pause();

    n = min(me->token, size);
    me->token -= n;
    return n;
}

// 归还令牌，取多了，要归还回去。
int mytbf_returntoken(mytbf_t *ptr, int size) {
    mytbf *me = ptr;

    if (size <= 0) return -EINVAL;

    me->token += size;
    if (me->token > me->burst)
        me->token = me->burst;
    return size;
}

/**
 * 将你使用过的令牌桶销毁，即你可能使用了其中一个速率
 * 的令牌，你要将它销毁，而一种速率的令牌只是数组中的
 * 一个元素，所以你只要将那个位置置为空即可。
 */
int mytbf_destroy(mytbf_t *ptr) {
    // 类型转换
    mytbf *me = ptr;
    job[me->pos] = NULL;
    free(ptr);
    return 0;
}
