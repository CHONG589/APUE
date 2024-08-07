#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <pthread.h>
#include <string.h>

#include "anytimer.h"

typedef struct anytimer_st {
    int sec;
    at_jobfunc_t *jobp;
    char *arg;
    int flag;         // running == 1/over == 0
    pthread_mutex_t mut;
} anytimer;

static anytimer *job[JOB_MAX];
static pthread_mutex_t mut_job = PTHREAD_MUTEX_INITIALIZER;
static pthread_once_t init_once = PTHREAD_ONCE_INIT;
static pthread_t tid_alrm;

static void *thr_alrm(void *p) {
    int i;
    while(1) {
        pthread_mutex_lock(&mut_job);
        for (i = 0; i < JOB_MAX; i++) {
            if (job[i] != NULL) {
                pthread_mutex_lock(&job[i]->mut);
                if ((job[i]->sec != 0) && (job[i]->flag == 1)) {
                    (job[i]->sec) -= 1;
                    if ((job[i]->sec == 0) && (job[i]->flag == 1)) {
                        job[i]->flag = 0;
                        (job[i]->jobp)(job[i]->arg);
                    }
                }
                pthread_mutex_unlock(&job[i]->mut);
            }
        }
        pthread_mutex_unlock(&mut_job);
        sleep(1);
    }
}

static void module_unload(void) {

    int i;

    pthread_cancel(tid_alrm);
    pthread_join(tid_alrm, NULL);

    for (i = 0; i < JOB_MAX; i++) {
        at_waitjob(i);
    }
    pthread_mutex_destroy(&mut_job);
}

static void module_load(void) {

    int err = pthread_create(&tid_alrm, NULL, thr_alrm, NULL);
    if (err) {
        fprintf(stderr, "pthread_create(): %s\n", strerror(err)); 
        exit(1);
    }

    atexit(module_unload);
}

static int get_free_pos_unlocked(void) {
    int i;
    for (i = 0; i < JOB_MAX; i++) {
        if (job[i] == NULL) return i;
    }
    return -1;
}

/**
 * 不用欺骗的形式来隐藏结构类型，而是模仿文件描述符的方式,
 * 直接返回数组下标。
 * 
 * return >= 0          成功，返回任务 ID
 * return == -EINVAL    失败，参数非法（sec 为负或 arg == NULL）
 * return == -ENOSPC    失败，数组满
 * return == -ENOMEM    失败，内存空间不足
 */
int at_addjob(int sec, at_jobfunc_t *jobp, char *arg) {

    if (sec < 0 || arg == NULL) {
        return -EINVAL;
    }

    pthread_once(&init_once, module_load);

    pthread_mutex_lock(&mut_job);
    int pos = get_free_pos_unlocked();
    if (pos < 0) {
        pthread_mutex_unlock(&mut_job);
        return -ENOSPC;
    }
    job[pos] = malloc(sizeof(anytimer));
    if (job[pos] == NULL) {
        pthread_mutex_unlock(&mut_job);
        return -ENOMEM;
    } 
    job[pos]->sec = sec;
    job[pos]->jobp = jobp;
    job[pos]->arg = arg;
    job[pos]->flag = 1; 
    pthread_mutex_unlock(&mut_job);

    pthread_mutex_init(&job[pos]->mut, NULL);

    return pos;
}

/**
 * 取消定时任务，但是不释放对应的空间，释放空间
 * at_waitjob 实现
 * 
 * return == 0          成功，指定任务成功取消
 * return == -EINVAL    失败，参数非法
 * return == -EBUSY     失败，指定任务已完成，sec 已经为0
 * return == -ECANCELED 失败，指定任务重复取消，flag 已经为 over
 */
int at_canceljob(int id) {

    if (id < 0) return -EINVAL;
    if (job[id]->sec == 0) return -EBUSY;
    if (job[id]->flag == 0) return -ECANCELED;

    pthread_mutex_lock(&job[id]->mut);
    job[id]->flag = 0;
    pthread_mutex_unlock(&job[id]->mut);

    return 0;
}

/**
 * 收尸操作，不能收回正在 running 的任务
 * 
 * return == 0          成功，指定任务成功释放
 * return == -EINVAL    失败，参数非法
 */
int at_waitjob(int id) {

    if (id < 0) return -EINVAL;

    pthread_mutex_lock(&mut_job);
    job[id] = NULL;
    pthread_mutex_unlock(&mut_job);

    pthread_mutex_destroy(&job[id]->mut);
    free(job[id]);
    return 0;
}
