#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <signal.h>
#include <unistd.h>
#include <sys/time.h>

#include "anytimer.h"

enum {
    STATE_RUNNING = 1,
    STATE_CANCELED,
    STATE_OVER
};

struct at_job_st {
    int job_state;
    int sec;// 保留原始的时间值，可以用来重复执行这个计数，也可以在出错时打印该值。
    int time_remain;// 真正用来时间递减的值
    int repeat;
    at_jobfunc_t *jobp;
    void *arg;
};

static struct at_job_st *job[JOB_MAX];
static int inited = 0;
static struct sigaction alrm_sa_save;

static void alrm_action(int s, siginfo_t *infop, void *unused) {

    if (infop->si_code != SI_KERNEL) {
        // 信号如果不是从内核态来的，那么就不响应
        return ;
    }

    int i;

    for (i = 0; i < JOB_MAX; i++) {
        if (job[i] != NULL && job[i]->job_state == STATE_RUNNING) {
            job[i]->time_remain--;
            if (job[i]->time_remain == 0) {
                job[i]->jobp(job[i]->arg);
                if (job[i]->repeat == 1) 
                    job[i]->time_remain = job[i]->sec;
                else
                    job[i]->job_state = STATE_OVER;
            }
        }
    }
}

static void module_unload(void) {

    int i;
    struct itimerval itv;
    
    // 采取直接将信号关掉，没有使用恢复旧值得操作
    itv.it_interval.tv_sec = 0;
    itv.it_interval.tv_usec = 0;
    itv.it_value.tv_sec = 0;
    itv.it_value.tv_usec = 0;
    if (setitimer(ITIMER_REAL, &itv, NULL) < 0) {
        perror("setitimer()");
        exit(1);
    }

    if (sigaction(SIGALRM, &alrm_sa_save, NULL) < 0) {
        perror("sigaction()");
        exit(1);
    }

    for (i = 0; i < JOB_MAX; i++) {
        free(job[i]);
    }
}

static void module_load(void) {

    struct sigaction sa;
    struct itimerval itv;

    sa.sa_sigaction = alrm_action;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_SIGINFO;// 说明是调用三个参数的那个，而不是一个参数的
    if (sigaction(SIGALRM, &sa, &alrm_sa_save) < 0) {
        perror("sigaction()");
        exit(1);
    }

    itv.it_interval.tv_sec = 1;
    itv.it_interval.tv_usec = 0;
    itv.it_value.tv_sec = 1;
    itv.it_value.tv_usec = 0;
    if (setitimer(ITIMER_REAL, &itv, NULL) < 0) {
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

    struct at_job_st *me;

    if (sec < 0 || arg == NULL) {
        return -EINVAL;
    }

    int pos = get_free_pos();
    if (pos < 0) {
        return -ENOSPC;
    }
    me = malloc(sizeof(*me));
    if (me == NULL) {
        return -ENOMEM;
    } 

    if (!inited) {
        module_load();
        inited = 1;
    }

    me->job_state = STATE_RUNNING;
    me->sec = sec;
    me->time_remain = me->sec;
    me->jobp = jobp;
    me->arg = arg;
    me->repeat = 0;
    job[pos] = me;
    //printf ("%d, %s, %d\n", job[pos]->sec, job[pos]->arg, job[pos]->flag);

    return pos;
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
int at_addjob_repeat(int sec, at_jobfunc_t *jobp, void *arg) {

    struct at_job_st *me;

    if (sec < 0 || arg == NULL) {
        return -EINVAL;
    }

    int pos = get_free_pos();
    if (pos < 0) {
        return -ENOSPC;
    }
    me = malloc(sizeof(*me));
    if (me == NULL) {
        return -ENOMEM;
    }

    if (!inited) {
        module_load();
        inited = 1;
    }

    me->job_state = STATE_RUNNING;
    me->sec = sec;
    me->time_remain = me->sec;
    me->jobp = jobp;
    me->arg = arg;
    me->repeat = 1;

    job[pos] = me;

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

    if (id < 0 || id >= JOB_MAX || job[id] == NULL) return -EINVAL;
    if (job[id]->job_state == STATE_OVER) return -EBUSY;
    if (job[id]->job_state == STATE_CANCELED) return -ECANCELED;

    job[id]->job_state = STATE_CANCELED;

    return 0;
}

/**
 * 收尸操作
 * 
 * return == 0          成功，指定任务成功释放
 * return == -EINVAL    失败，参数非法
 * return == -EBUSY     失败，指定任务为周期性任务
 */
int at_waitjob(int id) {

    if (id < 0 || id >= JOB_MAX || job[id] == NULL) return -EINVAL;

    if (job[id]->repeat == 1) {
        // 周期性任务不能收尸
        return -EBUSY;
    }

    while (job[id]->job_state == STATE_RUNNING) {
        // 如果此时为运行态，则要阻塞，等到不是的时候
        // 再来收尸
        // 这里设置了每秒都有信号，所以每秒都会来查看状态
        pause();
    }

    if (job[id]->job_state == STATE_CANCELED || job[id]->job_state == STATE_OVER) {
        free(job[id]);
        job[id] = NULL;
    }

    job[id] = NULL;
    free(job[id]);

    return 0;
}
