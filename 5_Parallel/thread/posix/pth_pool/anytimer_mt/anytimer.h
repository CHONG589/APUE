#ifndef ANYTIMER_H_
#define ANYTIMER_H_

#define JOB_MAX     1024
typedef void at_jobfunc_t(char *);

/**
 * 不用欺骗的形式来隐藏结构类型，而是模仿文件描述符的方式,
 * 直接返回数组下标。
 * 
 * return >= 0          成功，返回任务 ID
 * return == -EINVAL    失败，参数非法（sec 为负或 arg == NULL）
 * return == -ENOSPC    失败，数组满
 * return == -ENOMEM    失败，内存空间不足
 */
int at_addjob(int sec, at_jobfunc_t *jobp, char *arg);

/**
 * 取消定时任务，但是不释放对应的空间，释放空间
 * at_waitjob 实现
 * 
 * return == 0          成功，指定任务成功取消
 * return == -EINVAL    失败，参数非法
 * return == -EBUSY     失败，指定任务已完成
 * return == -ECANCELED 失败，指定任务重复取消
 */
int at_canceljob(int id);

/**
 * 收尸操作
 * 
 * return == 0          成功，指定任务成功释放
 * return == -EINVAL    失败，参数非法
 */
int at_waitjob(int id);

#endif
