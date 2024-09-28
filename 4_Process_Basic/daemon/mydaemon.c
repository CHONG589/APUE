#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <syslog.h>
#include <errno.h>
#include <string.h>

#define FNAME   "/tmp/out"

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

int main() {

    FILE *fp;
    int i;

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

    fclose(fp);
    closelog();

    exit(0);
}
