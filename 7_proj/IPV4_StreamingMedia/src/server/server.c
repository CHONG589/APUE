#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <syslog.h>
#include <errno.h>
#include <string.h>
#include <signal.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <net/if.h>

#include "server_conf.h"
#include "medialib.h"
#include "thr_channel.h"
#include "thr_list.h"
#include <proto.h> 

/**
 * -M           指定多播组
 * -P           指定接收端口
 * -F           前台运行
 * -D           指定媒体库位置
 * -I           指定网络设备
 * -H           显示帮助
 */

struct server_conf_st server_conf = {.rcvport = DEFAULT_RCVPORT, \
                        .mgroup = DEFAULT_MGROUP, \
                        .media_dir = DEFAULT_MEDIADIR, \
                        .runmode = RUN_DAEMON, \
                        .ifname = DEFAULT_IF};

// 不能局部化
int serversd;
struct sockaddr_in sndaddr;

static struct mlib_listentry_st *list;

static void printhelp(void) {

    printf("-M 或 --mgroup          指定多播组\n");
    printf("-P 或 --port            指定接收端口\n");
    printf("-F 或 --foreground      前台运行\n");
    printf("-D 或 --media_dir       指定媒体库位置\n");
    printf("-I 或 --ifname          指定网络设备\n");
    printf("-H 或 --help            显示帮助\n");
}

static void daemon_exit(int s) {

    thr_list_destroy();
    thr_channel_destroyall();
    mlib_freechnlist(list);
    syslog(LOG_WARNING, "signal-%d caught, exit now.", s);
    closelog();

    exit(0);
}

static int daemonize(void) {

    pid_t pid;
    int fd;

    pid = fork();
    if (pid < 0) {
        // perror("fork()");
        syslog(LOG_ERR, "fork() : %s", strerror(errno));
        return -1;
    }
    if (pid > 0) {
        exit(0);
    }
    fd = open("/dev/null", O_RDWR);
    if (fd < 0) {
        // perror("open()");
        syslog(LOG_WARNING, "open() : %s", strerror(errno));
        return -2;
    }
    else {
        dup2(fd, 0);
        dup2(fd, 1);
        dup2(fd, 2);

        if (fd > 2) {
            // 防止打开的文件返回的的 fd 是 0，1，2
            close(fd);
        }
    }

    setsid();

    chdir("/");
    umask(0);

    return 0;
}

static int socket_init(void) {

    struct ip_mreqn mreq;

    serversd = socket(AF_INET, SOCK_DGRAM, 0);
    if (serversd < 0) {
        syslog(LOG_ERR, "socket() : %s", strerror(errno));
        exit(1);
    }

    inet_pton(AF_INET, server_conf.mgroup, &mreq.imr_multiaddr);
    inet_pton(AF_INET, "0.0.0.0", &mreq.imr_address);
    mreq.imr_ifindex = if_nametoindex(server_conf.ifname);
    if (setsockopt(serversd, IPPROTO_IP, IP_MULTICAST_IF, &mreq, sizeof(mreq)) < 0) {
        syslog(LOG_ERR, "setsockopt(IP_MULTICAST_IF) : %s", strerror(errno));
        exit(1);
    }

    // bind();

    sndaddr.sin_family = AF_INET;
    sndaddr.sin_port = htons(atoi(server_conf.rcvport));
    inet_pton(AF_INET, server_conf.mgroup, &sndaddr.sin_addr.s_addr);

    return 0;
}

int main(int argc, char *argv[]) {

    int c;
    int index = 0;
    struct sigaction sa;
    struct option argarr[] = {{"mgroup", 1, NULL, 'M'}, {"port", 1, NULL, 'P'}, \
                                {"foreground", 0, NULL, 'F'}, {"media_dir", 1, NULL, 'D'}, \
                                {"ifname", 1, NULL, 'I'}, {"help", 0, NULL, 'H'}, \
                                {NULL, 0, NULL, 0}};

    // 修改下面三种信号来了后的行为，当这三种信号来了后
    // 执行清理操作，即释放资源。
    sa.sa_handler = daemon_exit;
    sigemptyset(&sa.sa_mask);
    sigaddset(&sa.sa_mask, SIGINT);
    sigaddset(&sa.sa_mask, SIGQUIT);
    sigaddset(&sa.sa_mask, SIGTERM);
    sigaction(SIGTERM, &sa, NULL);
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGQUIT, &sa, NULL);

    openlog("netradio", LOG_PID | LOG_PERROR, LOG_DAEMON);

    // 命令行分析
    while (1) {

        c = getopt_long(argc, argv, "M:P:FD:I:H", argarr, &index);
        if (c < 0) break;
        switch(c) {
            case 'M':
                server_conf.mgroup = optarg;
                break;
            case 'P':
                server_conf.rcvport = optarg;
                break;
            case 'F':
                server_conf.runmode = RUN_FOREGROUN;
                break;
            case 'D':
                server_conf.media_dir = optarg;
                break;
            case 'I':
                server_conf.ifname = optarg;
                break;
            case 'H':
                printhelp();
                exit(0);
                break;
            default:
                abort();
                break;
        }
    }

    // 守护进程的实现
    if (server_conf.runmode == RUN_DAEMON) {
        if (daemonize() != 0) {
            exit(1);
        }
    }
    else if (server_conf.runmode == RUN_FOREGROUN) {
        // 前台进程
        // do nothing
    }
    else {
        // fprintf(stderr, "EINVAL\\n");
        syslog(LOG_ERR, "EINVAL server_conf.runmode.");
        exit(1);
    }

    // SOCKET 初始化
    socket_init();

    // 获取频道信息
    int list_size;
    int err;

    // 获取有哪些频道，然后再给对应的频道建立线程。
    err = mlib_getchnlist(&list, &list_size);
    if (err) {
        syslog(LOG_ERR, "mlib_getchnlist() : %s.", strerror(err));
        exit(1);
    }

    // 创建节目单线程
    thr_list_create(list, list_size);
    if (err)
        exit(1);

    // 创建频道线程
    int i;

    for (i = 0; i < list_size; i++) {
        err = thr_channel_create(list + i);
        if (err) {
            fprintf(stderr, "thr_channel_create() : %s\n", strerror(errno));
            exit(1);
        }
    }

    syslog(LOG_DEBUG, "%d channel threads created.", i);

    while(1)
        pause();

    exit(0);
}
