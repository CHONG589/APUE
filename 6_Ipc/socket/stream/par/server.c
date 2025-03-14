#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

#include "proto.h"

#define IPSTRSIZE       40
#define BUFSIZE         1024

static void server_job(int sd) {

    char buf[BUFSIZE];
    int len;

    len = sprintf(buf, FMT_STAMP, (long long)time(NULL));
    if (send(sd, buf, len, 0) < 0) {
        perror("send()");
        exit(1);
    }
}

int main() {

    int sd, newsd;
    pid_t pid;
    struct sockaddr_in laddr, raddr;
    socklen_t raddr_len;
    char ipstr[IPSTRSIZE];

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

    raddr_len = sizeof(raddr);
    while(1) {

        newsd = accept(sd, (void *)&raddr, &raddr_len);
        if (newsd < 0) {
            perror("accept()");
            exit(1);
        }

        pid = fork();
        if (pid < 0) {
            perror("fork()");
            exit(1);
        }
        if (pid == 0) {
            // 子进程
            // 子进程只需要 newsd 的，而不需要 sd，父进程
            // 才需要 sd。
            close(sd);

            inet_ntop(AF_INET, &raddr.sin_addr, ipstr, IPSTRSIZE);
            printf("Client：%s：%d\n", ipstr, ntohs(raddr.sin_port));
            server_job(newsd);
            close(newsd);
            exit(0);
        }
        // 对于为什么上面关了 newsd，这里为啥还要关掉，因为 newsd
        // 的产生在 fork 之前，所以子进程和父进程都会有一份 newsd，
        // 因为子进程创建时会复制，所以你只是把子进程的 newsd 关了，
        // 这里还要把父进程的 newsd 关了。
        close(newsd);
    }

    close(sd);

    exit(0);
}
