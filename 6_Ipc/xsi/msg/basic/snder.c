// 负责发送询问信息

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <string.h>

#include "proto.h"

int main() {

    key_t key;
    struct msg_st sbuf;
    int msgid;

    key = ftok(KEYPATH, KEYPROJ);
    if (key < 0) {
        perror("ftok()");
        exit(1);
    }

    msgid = msgget(key, 0);
    if (msgid < 0) {
        perror("msgget()");
        exit(1);
    }

    sbuf.mtype = 1;// 不能小于 0
    strcpy(sbuf.name, "Alan");// 数组名不能用赋值的方式，只能用拷贝 
    sbuf.math = rand() % 100;
    sbuf.chinese = rand() % 100;
    if (msgsnd(msgid, &sbuf, sizeof(sbuf) - sizeof(long), 0) < 0) {
        perror("msgsnd()");
        exit(1);
    }

    // 这里没有创建，所以没有销毁步骤，创建在接收的那里，所以
    // 理应由接受的那里销毁。

    puts("ok!");

    exit(0);
}
