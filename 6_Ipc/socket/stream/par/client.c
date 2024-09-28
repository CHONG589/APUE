#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

#include "proto.h"

int main(int argc, char *argv[]) {

    int sd;
    struct sockaddr_in raddr;
    long long stamp;
    FILE *fp;

    if (argc < 2) {
        fprintf(stderr, "Usage....\n");
        exit(1);
    }

    sd = socket(AF_INET, SOCK_STREAM, 0);
    if (sd < 0) {
        perror("socket()");
        exit(1);
    }

    // bind();

    raddr.sin_family = AF_INET;
    raddr.sin_port = htons(atoi(SERVERPORT));
    inet_pton(AF_INET, argv[1], &raddr.sin_addr);
    if (connect(sd, (void *)&raddr, sizeof(raddr)) < 0) {
        perror("connect()");
        exit(1);
    }

    // rcve();
    // close();

    // 为了可移植性，因为文件 IO 不可移植，所以这里将一个文件
    // 描述符相关的操作，加上权限的指定，把它转换成流的一个操作
    // 这样对 socket 的使用完全转换成了标准 IO 的使用。
    fp = fdopen(sd, "r+");
    if (fp == NULL) {
        perror("fdopen()");
        exit(1);
    }

    // 从 fp 中读取数据。
    if (fscanf(fp, FMT_STAMP, &stamp) < 1) {
        // 返回值为输入的个数, 这里输入只有 stamp 一个。
        fprintf(stderr, "Bad format!\n");
    }
    else {
        fprintf(stdout, "stamp = %lld\n", stamp);
    }

    fclose(fp);

    exit(0);
}
