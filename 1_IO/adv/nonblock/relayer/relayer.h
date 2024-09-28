#ifndef RELAYER_H__
#define RELAYER_H__

#define REL_JOBMAX      10000

enum {
    STATE_RUNNING = 1,
    STATE_CANCELED,
    STATE_OVER
};

typedef struct rel_stat_st {
    int state;      //记录任务的状态
    int fd1;        // 记录是哪个文件
    int fd2;
    int64_t count12, count21;// 分别记录 1 向 2 和 2 向 1 的对话次数(还是字符个数？)。 
    // struct timerval start, end;
} rel_stat;

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
int rel_addjob(int fd1, int fd2); 

/**
 * 取消任务
 * 
 * return == 0          成功，指定任务成功取消
 *        == -EINVAL    失败，参数非法
 *        == -EBUSY     失败，任务早已被取消
 */
int rel_canceljob(int id);

/**
 * 收尸，同时可以返回任务的状态，第二个参数可以接收该任务
 * 的状态
 * 
 * return == 0          成功，指定任务已终止并返回状态
 *        == -EINVAL    失败，参数非法
 */
int rel_waitjob(int id, rel_stat *);

/**
 * 获取指定任务的状态
 * 
 * return == 0          成功，指定任务状态已经返回
 *        == -EINVAL    失败，参数非法
 */
int rel_statjob(int id, rel_stat *);

#endif
