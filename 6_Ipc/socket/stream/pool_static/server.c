#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <wait.h>

#include "proto.h"

#define IPSTRSIZE       40
#define BUFSIZE         1024
#define PROCNUM         4

static void server_job(int sd) {

    char buf[BUFSIZE];
    int len;

    len = sprintf(buf, FMT_STAMP, (long long)time(NULL));
    if (send(sd, buf, len, 0) < 0) {
        perror("send()");
        exit(1);
    }
}

static void server_loop(int sd) {

    int newsd;
    struct sockaddr_in raddr;
    socklen_t raddr_len;
    char ipstr[IPSTRSIZE];

    raddr_len = sizeof(raddr);
    while(1) {

        // accept 本身就实现互斥操作。
        newsd = accept(sd, (void *)&raddr, &raddr_len);
        if (newsd < 0) {
            perror("accept()");
            exit(1);
        }
        inet_ntop(AF_INET, &raddr.sin_addr, ipstr, IPSTRSIZE);
        printf("[%d] Client: %s: %d\n", getpid(), ipstr, ntohs(raddr.sin_port));
        server_job(newsd);
        close(newsd);
    }
    // 创建的每个子进程肯定都有各自的一个和父进程一样的
    // sd，复制的，所以各自也要关掉。
    // 纠正：sd 没有必要关，关 newsd 就可以。
    // close(sd);
}

int main() {

    int sd;
    struct sockaddr_in laddr;
    pid_t pid;
    int i;

    sd = socket(AF_INET, SOCK_STREAM, 0/*IPPROTO_TCP*/);
    if (sd < 0) {
        perror("socket()");
        exit(1);
    }

    // 该程序会处于 while 中，结束时，在终端用 Ctrl + C 杀死
    // 程序，程序中的 close 不会运行到，这会使下次运行出现 bind 
    // 不上的情况，因为端口被占用，所以这里要设置一杀死，就清理掉。
    int val = 1;
    if (setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val)) < 0) {
        perror("setsockopt()");
        exit(1);
    }

    laddr.sin_family = AF_INET;
    laddr.sin_port = htons(atoi(SERVERPORT));
    inet_pton(AF_INET, "0.0.0.0", &laddr.sin_addr);

    if (bind(sd, (void *)&laddr, sizeof(laddr)) < 0) {
        perror("bind()");
        exit(1);
    }

    if (listen(sd, 200) < 0) {
        perror("listen()");
        exit(1);
    }

    for(i = 0; i < PROCNUM; i++) {
        pid = fork();
        if (pid < 0) {
            perror("fork()");
            exit(1);
        }
        if (pid == 0) {

            // 创建的子进程不是干完一个任务就结束，而是干完一个
            // 后继续干，直到没有任务才终止进程。
            server_loop(sd);
            exit(0);
        }
    }

    // 父进程要做的事情。
    for(i = 0; i < PROCNUM; i++) {
        wait(NULL);
    }

    close(sd);
    exit(0);

}
