#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <syslog.h>
#include <errno.h>
#include <string.h>
#include <signal.h>

#define FNAME   "/tmp/out"

static FILE *fp;

static int  daemonize(void) {

    pid_t pid;
    int fd;

    // 只能子进程调用 setsid
    pid = fork();
    if (pid < 0) {
        return -1;
    }
    if (pid > 0) {
        // 父进程正常结束，让子进程成为孤儿
        exit(0);
    }

    // 实现守护进程的条件，关闭标准输入输出
    fd = open("/dev/null", O_RDWR);
    if (fd < 0) {
        return -1;
    }

    // 即将 /dev/null 重定向到当前进程的文件描述符的前三个位置。
    dup2(fd, 0);
    dup2(fd, 1);
    dup2(fd, 2);
    if (fd > 2)
        close(fd);

    // 子进程来创建守护进程。
    setsid();

    // 收尾工作
    // 将当前工作路径设置到根目录。
    chdir("/");
    // 给最大的权限。
    // umask(0);

    return 0;
}

/**
 * 由于本程序中使用的三个信号 SIGINT，SIGQUIT，SIGTERM
 * 的信号处理函数都是用的同一个，所以没进行区分，要是不
 * 一样，则要进行判断，分别执行对应的函数了。而 s 就是用
 * 来判断是哪个信号的。
 */
static void daemon_exit(int s) {

    // if (s == SIGINT) {

    // }
    // else if (s == SIGQUIT) {

    // }
    // else if (s == SIGTERM) {

    // }

    fclose(fp);
    closelog();
    exit(0);
}

int main() {

    int i;
    struct sigaction sa;

    // 有重入问题
    // signal(SIGINT, daemon_exit);
    // signal(SIGQUIT, daemon_exit);
    // signal(SIGTERM, daemon_exit);

    // 对于 SIGINT 信号的
    sa.sa_handler = daemon_exit;
    // 做成空集
    sigemptyset(&sa.sa_mask);
    // 将三个要用到的信号加入到 sa 中
    sigaddset(&sa.sa_mask, SIGQUIT);
    sigaddset(&sa.sa_mask, SIGTERM);
    sigaddset(&sa.sa_mask, SIGINT);
    sa.sa_flags = 0;

    // 分别响应对应的信号
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGQUIT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);

    openlog("mydaemon", LOG_PID, LOG_DAEMON);

    if (daemonize()) {
        syslog(LOG_ERR, "daemonize() failed!");
        exit(1);
    }
    else {
        syslog(LOG_INFO, "daemonize() successded!");
    }

    fp = fopen(FNAME, "w");
    if (fp == NULL) {
        // 将失败原因显示出来
        syslog(LOG_ERR, "fopen():%s", strerror(errno));
        exit(1);
    }

    syslog(LOG_INFO, "%s was opened.", FNAME);

    for (i = 0; ; i++) {
        fprintf(fp, "%d\n", i);
        fflush(fp);
        syslog(LOG_DEBUG, "%d is printed.", i);
        sleep(1);
    }

    exit(0);
}
