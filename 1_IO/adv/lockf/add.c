

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

#define PROCNUM      20
#define FNAME       "/tmp/out"
#define LINESIZE    1024

static void func_add(void) {

    FILE *fp;
    char linebuf[LINESIZE];
    int fd;

    fp = fopen(FNAME, "r+");
    if (fp == NULL) {
        perror("fopen");
        exit(1);
    }

    fd = fileno(fp);

    lockf(fd, F_LOCK, 0);
    fgets(linebuf, LINESIZE, fp);
    fseek(fp, 0, SEEK_SET);
    fprintf(fp, "%d\n", atoi(linebuf) + 1);
    fflush(fp);
    lockf(fd, F_ULOCK, 0);

    fclose(fp);

    return ;
}

int main() {

    pid_t pid;

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

    exit(0);
}
