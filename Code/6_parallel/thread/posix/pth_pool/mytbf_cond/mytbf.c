#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sched.h>

#include "mytbf.h"

typedef struct mytbf_st {
    int cps;
    int burst;
    int token;
    int pos;
    pthread_mutex_t mut;
    pthread_cond_t cond;
} mytbf;

static mytbf *job[MYTBF_MAX];
static pthread_mutex_t mut_job = PTHREAD_MUTEX_INITIALIZER;
static pthread_t tid_alrm;
static pthread_once_t init_once = PTHREAD_ONCE_INIT;

static void *thr_alrm(void *p) {
    int i;

    while(1) {
        pthread_mutex_lock(&mut_job);
        for (i = 0; i < MYTBF_MAX; i++) {
            if (job[i] != NULL) {
                pthread_mutex_lock(&job[i]->mut); 
                job[i]->token += job[i]->cps;
                if (job[i]->token > job[i]->burst)
                    job[i]->token = job[i]->burst;
                pthread_cond_broadcast(&job[i]->cond);
                pthread_mutex_unlock(&job[i]->mut);
            }
        }
        pthread_mutex_unlock(&mut_job);
        sleep(1);
    }
}

static void module_unload(void) {
   
    int i;

    // 结束线程。
    pthread_cancel(tid_alrm);
    pthread_join(tid_alrm, NULL);

    // 释放空间。
    for (i = 0; i < MYTBF_MAX; i++) {
        if (job[i] != NULL) {
            mytbf_destroy(job[i]);
        }
    }
    pthread_mutex_destroy(&mut_job);
}

static void module_load(void) {

    int err;
    err = pthread_create(&tid_alrm, NULL, thr_alrm, NULL);
    if (err) {
        fprintf(stderr, "pthread_create(): %s\n", strerror(err));
        exit(1);
    }

    atexit(module_unload);
}

static int get_free_pos_unlocked(void) {
    int i;
    for (i = 0; i < MYTBF_MAX; i++) {
        if (job[i] == NULL) return i;
    }
    return -1;
}

mytbf_t *mytbf_init(int cps, int burst) {
    mytbf *me;
    int pos;

    pthread_once(&init_once, module_load);

    me = malloc(sizeof(*me));
    if (me == NULL)
        return NULL;

    me->token = 0;  // 一定是从零开始
    me->cps = cps;
    me->burst = burst;
    pthread_mutex_init(&me->mut, NULL);
    pthread_cond_init(&me->cond, NULL);

    // 找空位，临界区
    pthread_mutex_lock(&mut_job);
    pos = get_free_pos_unlocked();
    if (pos < 0) {
        pthread_mutex_unlock(&mut_job);
        free(me);
        return NULL;
    } 

    me->pos = pos;
    job[pos] = me;
    pthread_mutex_unlock(&mut_job);

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

    if (size <= 0) return -EINVAL;

    // 没有时要等待。
    pthread_mutex_lock(&me->mut);
    while(me->token <= 0) {
        //当 token <= 0 时，me->mut 解锁，让其它线程能进去改变
        //token 值，然后这里等待信号（条件变量）通知（阻塞在条件
        //变量上），等到一个通知后，先抢锁，然后判断 token 是否成立
        pthread_cond_wait(&me->cond, &me->mut);

        // pthread_mutex_unlock(&me->mut);
        // sched_yield();
        // pthread_mutex_lock(&me->mut);
    }
    n = min(me->token, size);
    me->token -= n;
    pthread_mutex_unlock(&me->mut);

    return n;
}

// 归还令牌，取多了，要归还回去。
int mytbf_returntoken(mytbf_t *ptr, int size) {
    mytbf *me = ptr;

    if (size <= 0) return -EINVAL;

    pthread_mutex_lock(&me->mut);
    me->token += size;
    if (me->token > me->burst)
        me->token = me->burst;
    pthread_cond_broadcast(&me->cond);
    pthread_mutex_unlock(&me->mut);
    
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

    // 锁 job 是因为防止其它进程刚好判断出 ptr 不为空，想要
    // 操作它，然后还没操作就跳到这里然后把它给赋值 NULL，然后
    // 这样就变成操作空指针。
    pthread_mutex_lock(&mut_job);
    job[me->pos] = NULL;
    pthread_mutex_unlock(&mut_job);

    pthread_mutex_destroy(&me->mut);
    pthread_cond_destroy(&me->cond);
    free(ptr);
    return 0;
}
