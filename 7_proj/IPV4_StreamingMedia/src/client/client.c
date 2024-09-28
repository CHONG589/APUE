#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <errno.h>
#include <net/if.h>
#include <getopt.h>

#include "client.h"

// 可以不用这样写，在 makefile 中定义了路径
// #include ../include/proto.h"
#include "proto.h" 


/**
 * -M 或 --mgroup      指定多播组
 * -P 或 --port        指定接受端口
 * -p 或 --player      指定播放器
 * -H 或 --help        显示帮助
 */

struct client_conf_st client_conf = {\
        .rcvport = DEFAULT_RCVPORT,\
        .mgroup = DEFAULT_MGROUP,\
        .player_cmd = DEFAULT_PLAYERCMD};

static void printhelp(void) {

    printf("-P 或 --port        指定接收端口\n");
    printf("-M 或 --mgroup      指定多播组\n");
    printf("-p 或 --player      指定播放器\n");
    printf("-H 或 --help        显示帮助\n");
}

static ssize_t writen(int fd, const char *buf, size_t len) {

    int ret, pos;

    pos = 0;
    while(len > 0) {

        ret = write(fd, buf + pos, len);
        if (ret < 0) {
            if (errno == EINTR)
                continue;
            perror("write()");
            return -1; 
        }
        len -= ret;
        pos += ret;
    }
    return pos;
}

int main(int argc, char *argv[]) {

    /**
     * 初始化
     * 级别：默认值，配置文件，环境变量，命令行参数
     */

    int index = 0;
    int c, val, len, chosenid, ret;
    int sd;
    int pd[2];
    pid_t pid;
    struct ip_mreqn mreq;
    struct sockaddr_in laddr, serveraddr, raddr;
    socklen_t serveraddr_len, raddr_len;
    struct option argarr[] = {{"port", 1, NULL, 'P'}, {"mgroup", 1, NULL, 'M'}, \
                                {"player", 1, NULL, 'p'}, {"help", 0, NULL, 'H'}, \
                                {NULL, 0, NULL, 0}};

    // 分析命令行
    // https://www.cnblogs.com/shanyu20/p/15480748.html 函数解析
    while (1) {
        c = getopt_long(argc, argv, "P:M:p:H", argarr, &index);
        if (c < 0) break;
        switch(c) {
            case 'P':
                client_conf.rcvport = optarg;
                break;
            case 'M':
                client_conf.mgroup = optarg;
                break;
            case 'p':
                client_conf.player_cmd = optarg;
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

    sd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sd < 0) {
        perror("socket()");
        exit(1);
    }

    // 采用多播组的形式, client 端是采用加入多播组，而 server 端
    // 是创建多播组
    inet_pton(AF_INET, client_conf.mgroup, &mreq.imr_multiaddr);
    inet_pton(AF_INET, "0.0.0.0", &mreq.imr_address);
    mreq.imr_ifindex = if_nametoindex("ens33");
    if (setsockopt(sd, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq)) < 0) {
        perror("setsockopt()");
        exit(1);
    }

    // 设置或禁止多播数据回送，即多播的数据是否回送到本地回环接口（发送端）
    val = 1;
    if (setsockopt(sd, IPPROTO_IP, IP_MULTICAST_LOOP, &val, sizeof(val)) < 0) {
        perror("setsockopt()");
        exit(1);
    }

    // 这里 server 为主动端，所以这里的 client 端要 bind，
    // 而 server 可以不需要。
    laddr.sin_family = AF_INET;
    laddr.sin_port = htons(atoi(client_conf.rcvport));
    inet_pton(AF_INET, "0.0.0.0", &laddr.sin_addr.s_addr);
    if (bind(sd, (void *)&laddr, sizeof(laddr)) < 0) {
        perror("bind()");
        exit(1);
    }

    if (pipe(pd) < 0) {
        perror("pipe()");
        exit(1);
    }

    pid = fork();
    if (pid < 0) {
        perror("fork()");
        exit(1);
    }
    if (pid == 0) {
        // 子进程：调用解码器
        close(sd);
        close(pd[1]);// 关掉写端
        dup2(pd[0], 0);// 将读端作为标准输入的位置，即文件描述符 0
        if (pd[0] > 0) {
            // 如果成功将 pd[0] 重定向，那么可以关掉 pd[0],
            // 这时使用标准输入就是使用 pd[0]。
            close(pd[0]);
        }
        // 使用 shell 运行 client_conf.player_cmd 中存的命令
        execl("/bin/sh", "sh", "-c", client_conf.player_cmd, NULL);
        perror("execl()");
        exit(1);
    }

    // 父进程：从网络上收包，发送给子进程
    // 收节目单
    struct msg_list_st *msg_list;
    msg_list = malloc(MSG_LIST_MAX);
    if (msg_list == NULL) {
        perror("malloc()");
        exit(1);
    }

    serveraddr_len = sizeof(serveraddr);
    while(1) {

        len = recvfrom(sd, msg_list, MSG_LIST_MAX, 0, (void *)&serveraddr, &serveraddr_len);
        if (len < sizeof(struct msg_list_st)) {
            // 如果接收到的数据大小都小于节目单包的最小大小，那么说明出错
            fprintf(stderr, "message is too small.\n");
            continue;
        }
        if (msg_list->chnid != LISTCHNID) {
            // 不是节目单的包，即 0 号包。
            fprintf(stderr, "chnid is not match.\n");
            continue;
        }
        // 收到 0 号包，跳出进入下一环节，接收完成。
        break;
    }

    // 打印节目单，并选择频道
    struct msg_listentry_st *pos;
    for (pos = msg_list->entry; (char *)pos < (((char *)msg_list) + len); pos = (void *)(((char *)pos) + ntohs(pos->len))) {
        printf("channel %d: %s\n", pos->chnid, pos->desc);
    }
    // free(msg_list);

    while(1) {

        ret = scanf("%d", &chosenid);
        if (ret != 1) {
            exit(1);
        }
        else {
            break;
        }
    }
    fprintf(stdout, "chosenid = %d\n", ret);

    // 收频道包，发送给子进程
    struct msg_channel_st *msg_channel;

    msg_channel = malloc(MSG_CHANNEL_MAX);
    if (msg_channel == NULL) {
        perror("malloc()");
        exit(1);
    }

    raddr_len = sizeof(raddr);
    while(1) {

        len = recvfrom(sd, msg_channel, MSG_CHANNEL_MAX, 0, (void *)&raddr, &raddr_len);
        if (raddr.sin_addr.s_addr != serveraddr.sin_addr.s_addr \
            || raddr.sin_port != serveraddr.sin_port) {
            fprintf(stderr, "Ignore : address not match.\n");
            continue;
        }
        if (len < sizeof(struct msg_channel_st)) {
            fprintf(stderr, "Ignore : message too small.\n");
            continue;
        }
        if (msg_channel->chnid == chosenid) {
            fprintf(stdout, "accepted msg : %d recieved.\n", msg_channel->chnid);
            if (writen(pd[1], msg_channel->data, len - sizeof(chnid_t)) < 0) {
                exit(1);
            }
        }
    }

    free(msg_channel);
    close(sd);

    exit(0);
}
