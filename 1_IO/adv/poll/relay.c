#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <poll.h>

/**
 * 实现数据中继
 * 
 * 利用有限状态机的思想重构 mycopy
 */

// 两个终端设备
#define TTY1        "/dev/tty11"
#define TTY2        "/dev/tty12"

#define BUFSIZE     1024

/**
 * 对状态机封装成数据结构，这里有两个状态机
 */

enum {
    STATE_R = 1,
    STATE_W,
    STATE_AUTO,
    STATE_Ex,           // 异常态
    STATE_T             // 终止态
};

typedef struct fsm_st {
    int state;                  // 当前状态机状态
    int sfd;
    int dfd;
    char buf[BUFSIZE];          // 缓冲区
    int len;
    int pos;
    char *errstr;
} fsm;

static void fsm_driver(fsm *fsm) {

    int ret;

    switch(fsm->state) {

        case STATE_R:
            fsm->len = read(fsm->sfd, fsm->buf, BUFSIZE);
            if (fsm->len == 0) {
                fsm->state = STATE_T;
            } 
            else if (fsm->len < 0) {
                if (errno == EAGAIN) {
                    fsm->state = STATE_R;
                } 
                else {
                    fsm->state = STATE_Ex;
                    fsm->errstr = "read()";
                }
            } 
            else {
                fsm->pos = 0;
                fsm->state = STATE_W;
            }
            break;

        case STATE_W:
            ret = write(fsm->dfd, fsm->buf + fsm->pos, fsm->len);
            if (ret < 0) {
                if (errno == EAGAIN)
                    fsm->state = STATE_W;
                else {
                    fsm->state = STATE_Ex;
                    fsm->errstr = "write()";
                }
            }
            else {
                fsm->pos += ret;
                fsm->len -= ret;
                if (fsm->len == 0)
                    fsm->state = STATE_R;
                else 
                    fsm->state = STATE_W;
            }
            break;

        case STATE_Ex:
            perror(fsm->errstr);
            fsm->state = STATE_T;
            break;

        case STATE_T:
            //
            break;

        default:
            abort();
            break;
    }
}

static int max(int a, int b) {

    if (a > b) return a;
    return b;
}

static void relay(int fd1, int fd2) {

    int fd1_save;
    int fd2_save;
    fsm fsm12;
    fsm fsm21;
    struct pollfd pfd[2];

    // 保存文件旧的状态
    fd1_save = fcntl(fd1, F_GETFL);
    // 设置为非阻塞 IO，无论旧的状态是不是非阻塞 IO，都
    // 设置为非阻塞 IO。
    // 设置文件新的状态，将旧的状态再加上 nonblock 成为
    // 新的状态
    fcntl(fd1, F_SETFL, fd1_save | O_NONBLOCK);

    fd2_save = fcntl(fd2, F_GETFL);
    fcntl(fd2, F_SETFL, fd2_save | O_NONBLOCK);

    fsm12.state = STATE_R;
    fsm12.sfd = fd1;
    fsm12.dfd = fd2;
    fsm21.state = STATE_R;
    fsm21.sfd = fd2;
    fsm21.dfd = fd1;

    pfd[0].fd = fd1;
    pfd[1].fd = fd2;

    while((fsm12.state != STATE_T) || (fsm21.state != STATE_T)) {

        // 布置监视任务
        pfd[0].events = 0;   //位图清零
        // poll 是以 fd 为单位组织事件的
        if (fsm12.state == STATE_R) {
            // 这里说明 1 是可读的
            pfd[0].events |= POLLIN;
        }
        if (fsm21.state == STATE_W) {
            // 这里说明 1 可写
            pfd[0].events |= POLLOUT;
        }

        pfd[1].events = 0;
        if (fsm12.state == STATE_W) {
            // 这里表明 2 可写
            pfd[1].events |= POLLOUT;
        }
        if (fsm21.state == STATE_R) {
            // 这里表明 2 可读
            pfd[1].events |= POLLIN;
        }
        
        // 监视
        if (fsm12.state < STATE_AUTO || fsm21.state < STATE_AUTO) {

            while (poll(pfd, 2, -1) < 0) {
                if (errno == EINTR) {
                    continue;
                }
                perror("poll()");
                exit(1);
            }
        }

        // 查看监视结果
        if (pfd[0].revents & POLLIN || pfd[1].revents & POLLOUT \
            || fsm12.state > STATE_AUTO) {
                // 1 可读，2 可写时推 fsm12
                fsm_driver(&fsm12);
        }
        if (pfd[1].revents & POLLIN || pfd[0].revents & POLLOUT \
            || fsm21.state > STATE_AUTO) {
                // 2 可读，1 可写时推 fsm21
                fsm_driver(&fsm21);
        }
    }

    // 还原状态
    fcntl(fd1, F_SETFL, fd1_save);
    fcntl(fd2, F_SETFL, fd2_save);
}

int main() {

    int fd1, fd2;

    // 阻塞打开
    fd1 = open(TTY1, O_RDWR);
    if (fd1 < 0) {
        perror("open()");
        exit(1);
    }
    write(fd1, "TTY1\n", 5);

    // 非阻塞打开
    fd2 = open(TTY2, O_RDWR | O_NONBLOCK);
    if (fd2 < 0) {
        perror("open()");
        exit(1);
    }
    write(fd2, "TTY2\n", 5);

    relay(fd1, fd2);

    close(fd2);
    close(fd1);

    exit(0);
}
