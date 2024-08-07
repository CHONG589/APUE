#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#include "mysem.h"

typedef struct mysem_st {
    int value;               // 资源总量，即上限
    pthread_mutex_t mut;     // 改变 value 值需要锁
    pthread_cond_t cond;     // value 不够时阻塞。
} mysem;

mysem_t *mysem_init(int initval) {

    mysem *me;

    me = malloc(sizeof(*me));
    if (me == NULL) return NULL;

    me->value = initval;
    pthread_mutex_init(&me->mut, NULL);
    pthread_cond_init(&me->cond, NULL);

    return me;
}

/**
 * 归还资源量
 * 将第二个参数的数量归还到第一个参数中
 */
int mysem_add(mysem_t *ptr, int n) {

    mysem *me = ptr;

    pthread_mutex_lock(&me->mut);
    me->value += n;
    // 这里归还的数量，可能满足多个线程的要求，所以使用
    // broadcast
    pthread_cond_broadcast(&me->cond);
    pthread_mutex_unlock(&me->mut);
    return n;

}

/**
 * 取资源量
 * 从第一个参数中的资源取第二个参数中的数量
 * 
 * 注意：如果该资源只有一个，其中一个线程需要取三个，另一个
 * 线程需要取一个，那么这时应该分配给取一个的，如果分配给取
 * 三个的，那么两个线程都要等待，这样可能会导致死锁。即要求
 * 该资源值大于等于要取的数量时，才可以分配。
 */
int mysem_sub(mysem_t *ptr, int n) {

    mysem *me = ptr;

    pthread_mutex_lock(&me->mut);
    while(me->value < n) {
        pthread_cond_wait(&me->cond, &me->mut);
    }
    me->value -= n;
    pthread_mutex_unlock(&me->mut);

    return n;

}

int mysem_destroy(mysem_t *ptr) {

    mysem *me = ptr;

    pthread_mutex_destroy(&me->mut);
    pthread_cond_destroy(&me->cond);
    free(me);

    return 0;

}




