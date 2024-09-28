#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

#include "proto.h"

#define IPSTRSIZE       40

int main() {

    int sd;
    struct sockaddr_in laddr, raddr;
    struct msg_st rbuf;
    socklen_t raddr_len;
    char ipstr[IPSTRSIZE];

    // 0 表示在 AF_INET 协议族中实现 DGRAM 的默认方式
    sd = socket(AF_INET, SOCK_DGRAM, 0/*IPPROTO_UDP*/);// 取得 socket
    if (sd < 0) {
        perror("socket()");
        exit(1);
    }

    laddr.sin_family = AF_INET;
    // 约定在本地的端口是给我用的
    laddr.sin_port = htons(atoi(RCVPORT));
    // sin_addr 表示一个 IP 地址，此种这里要存的是二进制形式
    // 的 IP 地址，而不是我们习惯看的点分形式，所以我们要将
    // IP 地址转换成二进制，用到了 inet_pton 函数，第一个参数是
    // 协议族，第二个参数是点分形式的 IP 地址，第三个参数是转换
    // 成二进制后的存储地址，因为 IP 地址是不断变化的，所以这里
    // 采用 O.0.0.0，表示能够匹配任何地址。
    inet_pton(AF_INET, "0.0.0.0", &laddr.sin_addr);

    // 第二个参数是地址，第三个参数是地址的长度。
    //
    if (bind(sd, (void *)&laddr, sizeof(laddr)) < 0) {// 给 socket 取得地址
        perror("bind()");
        exit(1);
    }

    raddr_len = sizeof(raddr); 

    while(1) {

        // raddr 是要回填带回来的地址，因为我要记录对方的地址
        recvfrom(sd, &rbuf, sizeof(rbuf), 0, (void *)&raddr, &raddr_len);// 接收消息
        inet_ntop(AF_INET, &raddr.sin_addr, ipstr, IPSTRSIZE);
        printf("---MESSAGE FROM %s: %d---\n", ipstr, ntohs(raddr.sin_port));
        // name 是单字节的，不涉及大端小端存储
        printf("NAME = %s\n", rbuf.name);
        printf("MATH =  %d\n", ntohl(rbuf.math));
        printf("CHINESE = %d\n", ntohl(rbuf.chinese));
    }

    close(sd);// 关闭 socket

    exit(0);
}
