#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <signal.h>
#include <unistd.h>
#include <sys/time.h>

#include "anytimer.h"

typedef struct anytimer_st {
    int sec;
    at_jobfunc_t *jobp;
    char *arg;
    int flag;         // running == 1/over == 0
} anytimer;

static anytimer *job[JOB_MAX];
static int inited = 0;
static __sighandler_t alrm_handler_save;
static struct itimerval itv_old;
static struct itimerval itv;

static void alrm_handler(int s) {
    int i;
    //alarm(1);
    for (i = 0; i < JOB_MAX; i++) {
        if (job[i] != NULL) {
            if ((job[i]->sec != 0) && (job[i]->flag == 1)) {
                (job[i]->sec) -= 1;
            }
            if ((job[i]->sec == 0) && (job[i]->flag == 1)) {
                job[i]->flag = 0;
                (job[i]->jobp)(job[i]->arg);
            }
        }
    }
}

static void module_unload(void) {
    int i;
    signal(SIGALRM, alrm_handler_save);
    //alarm(0);
    setitimer(ITIMER_REAL, &itv_old, NULL);

    for (i = 0; i < JOB_MAX; i++) {
        free(job[i]);
    }
}

static void module_load(void) {

    alrm_handler_save = signal(SIGALRM, alrm_handler);
    //alarm(1);

    itv.it_interval.tv_sec = 1;
    itv.it_interval.tv_usec = 0;
    itv.it_value.tv_sec = 1;
    itv.it_value.tv_usec = 0;
    if (setitimer(ITIMER_REAL, &itv, &itv_old) < 0) {
        perror("setitimer()");
        exit(1);
    }

    atexit(module_unload);
}

static int get_free_pos(void) {
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
    int pos = get_free_pos();
    if (pos < 0) {
        return -ENOSPC;
    }
    job[pos] = malloc(sizeof(anytimer));
    if (job[pos] == NULL) {
        return -ENOMEM;
    } 

    if (!inited) {
        module_load();
        inited = 1;
    }

    job[pos]->sec = sec;
    job[pos]->jobp = jobp;
    job[pos]->arg = arg;
    job[pos]->flag = 1; 
    //printf ("%d, %s, %d\n", job[pos]->sec, job[pos]->arg, job[pos]->flag);

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

    job[id]->flag = 0;

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

    job[id] = NULL;
    free(job[id]);
    return 0;
}
