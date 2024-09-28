#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#include "mypipe.h"

struct mypipe_st {
    int head;
    int tail;
    char data[PIPESIZE];
    int datasize;// 记录有效字节数
    int count_rd;
    int count_wr;
    pthread_mutex_t mut;
    pthread_cond_t cond;
};

mypipe_t *mypipe_init(void) {

    struct mypipe_st *me;

    me = malloc(sizeof(*me));
    if (me == NULL) 
        return NULL;

    me->head = 0;
    me->tail = 0;
    me->datasize = 0;
    me->count_rd = 0;
    me->count_wr = 0;
    pthread_mutex_init(&me->mut, NULL);
    pthread_cond_init(&me->cond, NULL);

    return me;
}

// 为线程确定身份：读者/写者
// 针对第一个参数中的管道，注册第二个参数中的身份。
int mypipe_register(mypipe_t *ptr, int opmap) {

    /*if error 判断身份真假*/ 
    pthread_mutex_lock(&me->mut);
    if (opmap & MYPIPE_READ)
        me->count_rd++;
    if (opmap & MYPIPE_WRITE)
        me->count_wr++;

    // 这个是通知下两句中的 cond_wait 的，因为在多
    // 线程执行下面 while 的代码时，阻塞在那里时，
    // 只要一有线程注册完身份，就通知查看一下下面的
    // 条件可不可以跳出。
    pthread_cond_broadcast(&me->cond);

    // 管道最少要凑齐一个读者和一个读者才能够实现。
    while (me->count_rd <= 0 || me->count_wr <= 0)
        pthread_cond_wait(&me->cond, &me->mut);
    pthread_mutex_unlock(&me->mut);

    return 0;
}

// 注销身份
int mypipt_unregister(mypipe_t *ptr, int opmap) {

    /*if error 判断身份真假 */
    pthread_mutex_lock(&me->mut);
    if (opmap & MYPIPE_RD)
        me->count_rd--;
    if (opmap & MYPIPE_WR)
        me->count_wr--;
    // count_wr 变化，要通知
    pthread_cond_broadcast(&me->cond);
    pthread_mutex_unlock(&me->mut);

    return 0;
}

static int next(int index) {

    index = (index + 1) % PIPESIZE;

    return index;
}

static int mypipe_readbyte_unlocked(struct mypipe_st *me, char *datap) {

    if (me->datasize <= 0)
        retrurn -1;

    *datap = me->data[me->head];
    me->head = next(me->head);
    me->datasize--;
    return 0;
}

int mypipe_read(mypipe_t *ptr, void *buf, size_t count) {

    struct mypipe_st *me = ptr;
    int i;

    pthread_mutex_lock(&me->mut);
 
    // 这里有一点需要注意，就是当管道空时，此时读者一定会阻塞在
    // 这里，要是这时也没有写者写时，那么这个读者就会一直阻塞在
    // 这里，典型的忙等，写者什么时候来是不确定的，是异步事件。
    // 那么就要对读者和写者加一个计数器和加一个确定是读者还是写
    // 者的函数。
    while(me->datasize <= 0 && me->count_wr > 0) {  // 没数据但是有写者时才要阻塞在 cond 中
        pthread_cond_wait(&me->cond, &me->mut);   // 所以这里通知不阻塞的条件有当 datasize 变了和count_wr 变了后就会来查看一下。
    }
    // 上面是阻塞的条件，但是跳出这个循环可以是没有数据且写者不存在导致跳出的。
    // 但是这样也是不满足的，这时没必要继续执行下去了，直接退出。要有数据才行。
    if (me->datasize <= 0 && me->count_wr <= 0) {
        pthread_mutex_unlock(&me->mut);
        return 0;
    }
 
    // 总共要读 count 个。
    for (i = 0; i < count; i++) {
        // 如果队列中不够读了，那么通过返回值可以看是否要跳出来
        if (mypipe_readbyte_unlocked(me, buf + i) != 0)
            break;
        mypipe_readbyte_unlocked(me, buf + i);
    }

    // 读了 i 个字节，那么此时有空位了，可以进行写操作，要通知
    // 写线程。
    pthread_cond_broadcast(&me->cond);
    pthread_mutex_unlock(&me->mut);

    // 能读到多少就返回多少，而不是一定要等读到 count 后再结束
    // 这个函数，如果没有读够，那么用户可以再次调用该函数，继续
    // 读剩下的，就跟 read 一样。
    return i;
}

static int mypipe_writebyte_unlocked(struct mypipe_st *me, char *datap) {

    if (me->datasize >= PIPESIZE)
        return -1;

    me->datap[me->tail] = *datap;
    me->tail = next(me->tail);
    me->datasize++;
    return 0;
}

int mypipe_write(mypipe_t *, void *buf, size_t count) {

    struct mypipe_st *me = ptr;
    int i;

    pthread_mutex_lock(&me->mut);

    while(me->datasize >= PIPESIZE && me->count_rd > 0) {
        pthread_cond_wait(&me->cond, &me->mut);
    }

    while(me->datasize >= PIPESIZE && me->count_rd <= 0) {
        pthread_mutex_unlock(&me->mut);
        return 0;
    }

    for (i = 0; i < count; i++) {
        if (mypipe_writebyte_unlocked(me, buf + i) != 0) {
            break;
        }
        mypipe_writebyte_unlocked(me, buf + i);
    }

    pthread_cond_broadcast(&me->cond);
    pthread_mutex_unlock(&me->mut);

    return i;
}

int mypipe_destroy(mypipt_t *ptr) {

    struct mypipe_st *me = ptr;
    
    pthread_mutex_destroy(&me->mut);
    pthread_cond_destroy(&me->cond);

    free(ptr);

    return 0;
}






