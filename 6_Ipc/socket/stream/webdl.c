#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

#define BUFSIZE     1024

int main(int argc, char *argv[]) {

    int sd;
    struct sockaddr_in raddr;
    long long stamp;
    FILE *fp;
    char rbuf[BUFSIZE];
    int len;

    if (argc < 2) {
        fprintf(stderr, "Usage....\n");
        exit(1);
    }

    sd = socket(AF_INET, SOCK_STREAM, 0);
    if (sd < 0) {
        perror("socket()");
        exit(1);
    }

    raddr.sin_family = AF_INET;
    raddr.sin_port = htons(80);
    inet_pton(AF_INET, argv[1], &raddr.sin_addr);
    if (connect(sd, (void *)&raddr, sizeof(raddr)) < 0) {
        perror("connect()");
        exit(1);
    }

    // 为了可移植性，因为文件 IO 不可移植，所以这里将一个文件
    // 描述符相关的操作，加上权限的指定，把它转换成流的一个操作
    // 这样对 socket 的使用完全转换成了标准 IO 的使用。
    fp = fdopen(sd, "r+");
    if (fp == NULL) {
        perror("fdopen()");
        exit(1);
    }

    // 向服务器发起请求，请求内容为 test.png
    fprintf(fp, "GET /test.png\r\n\r\n");
    fflush(fp);

    while(1) {

        len = fread(rbuf, 1, BUFSIZE, fp);
        if (len <= 0) break;
        fwrite(rbuf, 1, len, stdout);
    }

    fclose(fp);

    exit(0);
}
