#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <pthread.h>
#include <string.h>

#include "relayer.h"

#define BUFSIZE     1024

enum {
    STATE_R = 1,
    STATE_W,
    STATE_Ex,           // 异常态
    STATE_T             // 终止态
};

typedef struct rel_fsm_st {
    int state;                  // 当前状态机状态
    int sfd;
    int dfd;
    char buf[BUFSIZE];          // 缓冲区
    int len;
    int pos;
    char *errstr;
    int64_t count;             //记录这个状态机方向传输的次数
} rel_fsm;

typedef struct rel_job_st {
    int fd1;
    int fd2;
    int job_state;
    rel_fsm fsm12, fsm21;
    int fd1_save, fd2_save;// 保存每个任务各自的旧文件状态
    // struct timerval start, end;
} rel_job_st;

static rel_job_st *rel_job[REL_JOBMAX];
static pthread_mutex_t mut_rel_job = PTHREAD_MUTEX_INITIALIZER;
static pthread_once_t init_once = PTHREAD_ONCE_INIT;
static pthread_t tid_relayer;

static void fsm_driver(rel_fsm *fsm) {

    int ret;

    switch(fsm->state) {

        case STATE_R:
            fsm->len = read(fsm->sfd, fsm->buf, BUFSIZE);
            if (fsm->len == 0) {
                fsm->state = STATE_T;
            } 
            else if (fsm->len < 0) {
                if (errno == EAGAIN) {
                    fsm->state = STATE_R;
                } 
                else {
                    fsm->state = STATE_Ex;
                    fsm->errstr = "read()";
                }
            } 
            else {
                fsm->pos = 0;
                fsm->state = STATE_W;
            }
            break;

        case STATE_W:
            ret = write(fsm->dfd, fsm->buf + fsm->pos, fsm->len);
            if (ret < 0) {
                if (errno == EAGAIN)
                    fsm->state = STATE_W;
                else {
                    fsm->state = STATE_Ex;
                    fsm->errstr = "write()";
                }
            }
            else {
                fsm->pos += ret;
                fsm->len -= ret;
                if (fsm->len == 0)
                    fsm->state = STATE_R;
                else 
                    fsm->state = STATE_W;
            }
            break;

        case STATE_Ex:
            perror(fsm->errstr);
            fsm->state = STATE_T;
            break;

        case STATE_T:
            //
            break;

        default:
            abort();
            break;
    }
}

static void *thr_relayer(void *p) {

    while(1) {
        pthread_mutex_lock(&mut_rel_job);
        for (int i = 0; i < REL_JOBMAX; i++) {
            if (rel_job[i] != NULL) {
                if (rel_job[i]->job_state == STATE_RUNNING) {
                    fsm_driver(&rel_job[i]->fsm12);
                    fsm_driver(&rel_job[i]->fsm21);
                    if (rel_job[i]->fsm12.state == STATE_T && \
                        rel_job[i]->fsm21.state == STATE_T) {
                          rel_job[i]->job_state = STATE_OVER;  
                    }
                }
            }
        }
        pthread_mutex_unlock(&mut_rel_job);
    }
}

static void module_unload(void) {

    pthread_cancel(tid_relayer);
    pthread_join(tid_relayer, NULL);

    int i;
    for (i = 0; i < REL_JOBMAX; i++) {
        if (rel_job[i] != NULL) {
            rel_waitjob(i, NULL);
        }
    }
    pthread_mutex_destroy(&mut_rel_job);
}

static void module_load(void) {

    int err = pthread_create(&tid_relayer, NULL, thr_relayer, NULL);
    if (err) {
        fprintf(stderr, "pthread_create(): %s\n", strerror(err));
        exit(1);
    }

    atexit(module_unload);
}

static int get_free_pos_unlocked() {

    int i;
    for (i = 0; i < REL_JOBMAX; i++) {
        if (rel_job[i] == NULL)
            return i;
    }
    return -1;
}

/**
 * 往 JOB 中添加任务
 * 
 * 传入两个文件描述符，进行创建任务，
 * 返回任务的下标给用户。
 * 
 * return >= 0          成功，返回当前任务 ID
 *        == -EINVAL    失败，参数非法
 *        == -ENOSPC    失败，任务数组满
 *        == -ENOMEM    失败，内存分配有误
 */
int rel_addjob(int fd1, int fd2) {

    rel_job_st *me;
    int pos;

    pthread_once(&init_once, module_load);

    me = malloc(sizeof(*me));
    if (me == NULL) {
        return -ENOMEM;
    }

    me->fd1 = fd1;
    me->fd2 = fd2;
    me->job_state = STATE_RUNNING;

    me->fd1_save = fcntl(me->fd1, F_GETFL);
    fcntl(me->fd1, F_SETFL, me->fd1_save | O_NONBLOCK);
    me->fd2_save = fcntl(me->fd2, F_GETFL);
    fcntl(me->fd2, F_SETFL, me->fd2_save | O_NONBLOCK);

    me->fsm12.sfd = me->fd1;
    me->fsm12.dfd = me->fd2;
    me->fsm12.state = STATE_R;

    me->fsm21.sfd = me->fd2;
    me->fsm21.dfd = me->fd1;
    me->fsm21.state = STATE_R;

    pthread_mutex_lock(&mut_rel_job);
    pos = get_free_pos_unlocked();
    if (pos < 0) {
        pthread_mutex_unlock(&mut_rel_job);
        fcntl(me->fd1, F_SETFL, me->fd1_save);
        fcntl(me->fd2, F_SETFL, me->fd2_save);
        free(me);
        return -ENOSPC;
    }

    rel_job[pos] = me;
    pthread_mutex_unlock(&mut_rel_job);

    return pos;
} 

/**
 * 取消任务
 * 
 * return == 0          成功，指定任务成功取消
 *        == -EINVAL    失败，参数非法
 *        == -EBUSY     失败，任务早已被取消
 */
int rel_canceljob(int id) {

    if (id < 0) return -EINVAL;

    if (rel_job[id]->job_state == STATE_CANCELED)
        return -EBUSY;

    rel_job[id]->job_state = STATE_CANCELED;
    
    return 0;
}

// /**
//  * 收尸，同时可以返回任务的状态，第二个参数可以接收该任务
//  * 的状态
//  * 
//  * return == 0          成功，指定任务已终止并返回状态
//  *        == -EINVAL    失败，参数非法
//  */
int rel_waitjob(int id, rel_stat *job_state) {

    if (id < 0) return -EINVAL;

    job_state->fd1 = rel_job[id]->fd1;
    job_state->fd2 = rel_job[id]->fd2;
    job_state->state = rel_job[id]->job_state;

    fcntl(rel_job[id]->fd1, F_SETFL, rel_job[id]->fd1_save);
    fcntl(rel_job[id]->fd2, F_SETFL, rel_job[id]->fd2_save);
    rel_job[id]->job_state = STATE_OVER;
    rel_job[id]->fsm12.state = STATE_T;
    rel_job[id]->fsm21.state = STATE_T;

    return 0;
}

// /**
//  * 获取指定任务的状态
//  * 
//  * return == 0          成功，指定任务状态已经返回
//  *        == -EINVAL    失败，参数非法
//  */
int rel_statjob(int id, rel_stat *job_state) {

    if (id < 0) return -EINVAL; 

    job_state->fd1 = rel_job[id]->fd1;
    job_state->fd2 = rel_job[id]->fd2;
    job_state->state = rel_job[id]->job_state;

    pthread_mutex_lock(&mut_rel_job);
    rel_job[id] = NULL;
    pthread_mutex_unlock(&mut_rel_job);

    free(rel_job[id]);

    return 0;
}




