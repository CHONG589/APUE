#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

#include "proto.h"

int main(int argc, char *argv[]) {

    int sd;
    struct msg_st sbuf;
    struct sockaddr_in raddr;

    sd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sd < 0) {
        perror("socket()");
        exit(1);
    }

    // 打开广播标志
    int val = 1;
    if (setsockopt(sd, SOL_SOCKET, SO_BROADCAST, &val, sizeof(val)) < 0) {
        perror("setsockopt()");
        exit(1);
    }

    // 如果没使用 bind 函数，则在发送时使用随机的端口
    // bind();

    memset(&sbuf, '\0', sizeof(sbuf));
    strcpy(sbuf.name, "Alan");
    sbuf.math = htonl(rand() % 100);
    sbuf.chinese = htonl(rand() % 100);

    raddr.sin_family = AF_INET;
    raddr.sin_port = htons(atoi(RCVPORT));
    // raddr 是要发送数据到的程序的 IP 端口，即指定使用本地的哪个
    // 端口来发送，这个的 IP 地址使用终端的形式输入，由于未使用
    // bind 函数绑定，所以端口会随机分配。
    inet_pton(AF_INET, "255.255.255.255", &raddr.sin_addr);

    // 存放目的主机 IP 地址和端口信息
    if (sendto(sd, &sbuf, sizeof(sbuf), 0, (void *)&raddr, sizeof(raddr)) < 0) {
        perror("sendto()");
        exit(1);
    }

    puts("ok!");

    close(sd);

    exit(0);
}
