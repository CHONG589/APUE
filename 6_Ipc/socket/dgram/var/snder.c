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
    int size;
    struct msg_st *sbufp;
    struct sockaddr_in raddr;

    if (argc < 3) {
        fprintf(stderr, "Usage....\n");
        exit(1);
    }

    if (strlen(argv[2]) > NAMEMAX) {
        fprintf(stderr, "Name is too long!\n");
        exit(1);
    }

    size = sizeof(struct msg_st) + strlen(argv[2]);
    sbufp = malloc(size);
    if (sbufp == NULL) {
        perror("malloc()");
        exit(1);
    }

    sd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sd < 0) {
        perror("socket()");
        exit(1);
    }

    // 如果没使用 bind 函数，则在发送时使用随机的端口
    // bind();

    strcpy(sbufp->name, argv[2]);
    sbufp->math = htonl(rand() % 100);
    sbufp->chinese = htonl(rand() % 100);

    raddr.sin_family = AF_INET;
    raddr.sin_port = htons(atoi(RCVPORT));
    // raddr 是要发送数据到的程序的 IP 端口，即指定使用本地的哪个
    // 端口来发送，这个的 IP 地址使用终端的形式输入，由于未使用
    // bind 函数绑定，所以端口会随机分配。
    inet_pton(AF_INET, argv[1], &raddr.sin_addr);

    // 存放目的主机 IP 地址和端口信息
    if (sendto(sd, sbufp, size, 0, (void *)&raddr, sizeof(raddr)) < 0) {
        perror("sendto()");
        exit(1);
    }

    puts("ok!");

    free(sbufp);
    close(sd);

    exit(0);
}
