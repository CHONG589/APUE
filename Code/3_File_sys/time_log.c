/**
 * 该程序向指定文件中输出时间，格式为：
 * 2024-7-7 15：32：20
 * 2024-7-7 15：32：21
 * 2024-7-7 15：32：22
 * ......
 * 程序终止
 * 下次继续往该文件输出时间
 * 2024-7-7 15：42：32
 * ......
 */

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#define FNAME   "out"
#define BUFFSIZE    1024

int main() {

    FILE *fd;
    char buf[BUFFSIZE];
    int count = 0;
    struct  tm *tm;
    time_t stamp;

    // 追加读写，而不是用 "rw"，因为下次再写会被覆盖掉
    fd = fopen(FNAME, "a+");
    if (fd == NULL) {
        perror("fopen()");
        exit(1);
    }

    /**
     * 此循环是为了当输出的文件不是第一次写时，也就是以前写过了，由于我们是
     * 追加写，所以要先获取当前文件已经有多少行了，然后行号再从那开始。
     * 也可以用 getline() 函数，这里只是将缓冲区设计大一些，保证每次完整
     * 读取一行。1024 肯定大于那一行的字符的。
     * Reading stops after an EOF  or  a  new‐line.  If a newline is 
     * read, it is stored into the buffer. 
     */
    while (fgets(buf, BUFFSIZE, fd) != NULL) {
        count++;
    }

    while (1) {
        time(&stamp);
        tm = localtime(&stamp);
        fprintf(fd, "%-4d%d-%d-%d %d:%d:%d\n", ++count, \
                tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday, \
                tm->tm_hour, tm->tm_min, tm->tm_sec);

        // 除了终端是行缓冲，输入到文件中是全缓冲，除非缓冲区已满，不然不会
        // 刷新，所以要手动刷新。
        fflush(fd);
        sleep(1);
    }

    fclose(fd);
    exit(0);
}
