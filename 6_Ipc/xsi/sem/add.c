

/**
 * 对指定文件中的一个数进行运算，这里用二十个线程，每个线程
 * 都只做一件相同的事，就是对文件中的数加一。
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <errno.h>

#define PROCNUM      20
#define FNAME       "/tmp/out"
#define LINESIZE    1024

static int semid;

static void P(void) {

    struct sembuf op;// 资源个数，是一个数组

    op.sem_num = 0;// 信号下标
    op.sem_op = -1;// 表明我要使用一个该资源，即减一
    op.sem_flg = 0; 

    // 第二个参数是数组的起始位置，第三个参数
    // 用来表示这个数组的个数
    // 至于为什么要用一个循环在这里，因为没有
    // 锁成功，则要一直锁。
    while (semop(semid, &op, 1) < 0) {
        if (errno != EINTR || errno != EAGAIN) {
            perror("semop()");
            exit(1);
        }
    }
}

static void V(void) {

    struct sembuf op;

    op.sem_num = 0;
    op.sem_op = 1;// 加一
    op.sem_flg = 0;

    // 解锁一般不会失败，所以没循环了。
    if (semop(semid, &op, 1) < 0) {
        perror("semop()");
        exit(1);
    }
}

static void func_add(void) {

    FILE *fp;
    char linebuf[LINESIZE];
    int fd;

    fp = fopen(FNAME, "r+");
    if (fp == NULL) {
        perror("fopen");
        exit(1);
    }

    P();
    fgets(linebuf, LINESIZE, fp);
    fseek(fp, 0, SEEK_SET);
    fprintf(fp, "%d\n", atoi(linebuf) + 1);
    fflush(fp);
    V();

    fclose(fp);

    return ;
}

int main() {

    pid_t pid;

    // 由于是父子进程的通信，是有亲缘关系的，所以
    // key 值就不太重要了。

    // 第二个参数相当于要多少个锁
    semid = semget(IPC_PRIVATE, 1, 0600);
    if (semid < 0) {
        perror("semget()");
        exit(1);
    }

    // 即第二个参数是指你要用哪一个锁，这里使用第一个
    // 锁，而第四个参数是指这个锁有一个资源总量。
    // 当第三个参数为 SETVAL 时，第四个参数是指
    // 为资源设置总量值，具体可以看 man 手册，
    // 第四个参数为 val 时，要设置 SETVAL。
    if (semctl(semid, 0, SETVAL, 1) < 0) {
        perror("semctl()");
        exit(1);
    }

    for (int i = 0; i < PROCNUM; i++) {
        pid = fork();
        if (pid < 0) {
            perror("fork()");
            exit(1);
        }
        if (pid == 0) {
            func_add();
            exit(0);
        }
    }

    for (int i = 0; i < PROCNUM; i++) {
        wait(NULL);
    }

    semctl(semid, 0, IPC_RMID);// 销毁

    exit(0);
}
