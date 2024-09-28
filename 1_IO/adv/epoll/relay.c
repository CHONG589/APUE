#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <sys/epoll.h>

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
    int epfd;
    struct epoll_event ev;

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

    epfd = epoll_create(10);
    if (epfd < 0) {
        perror("epoll_create()");
        exit(1);
    }

    ev.events = 0;   //暂时设置为没有行为，进入循环后添加行为
    ev.data.fd = fd1;
    // 相当于为 fd1 在 epfd 实例中注册了一个位置，后面要增加
    // 事件的时候直接用 fd1 在 epfd 中修改就行
    epoll_ctl(epfd, EPOLL_CTL_ADD, fd1, &ev);

    ev.events = 0;
    ev.data.fd = fd2;
    epoll_ctl(epfd, EPOLL_CTL_ADD, fd2, &ev);

    while((fsm12.state != STATE_T) || (fsm21.state != STATE_T)) {

        // 布置监视任务
        //进行修改创建的 epoll 实例 
        // 由于要一直循环，所以这里还要初始化
        ev.data.fd = fd1;
        ev.events = 0;

        if (fsm12.state == STATE_R) {
            ev.events |= EPOLLIN;
        }
        if (fsm21.state == STATE_W) {
            ev.events |= EPOLLOUT;
        }
        // 将添加到 ev 中的事件添加到 fd1 中，修改 fd1 中
        // 的事件
        epoll_ctl(epfd, EPOLL_CTL_MOD, fd1, &ev);

        ev.data.fd = fd2;
        ev.events = 0;
        if (fsm12.state == STATE_W) {
            ev.events |= EPOLLOUT;
        }
        if (fsm21.state == STATE_R) {
            ev.events |= EPOLLIN;
        }
        epoll_ctl(epfd, EPOLL_CTL_MOD, fd2, &ev);
        
        // 监视
        if (fsm12.state < STATE_AUTO || fsm21.state < STATE_AUTO) {

            while (epoll_wait(epfd, &ev, 1, -1) < 0) {
                if (errno == EINTR) {
                    continue;
                }
                perror("epoll_wait()");
                exit(1);
            }
        }

        // 查看监视结果
        if (ev.data.fd == fd1 && ev.events & EPOLLIN \
            || ev.data.fd == fd2 && ev.events & EPOLLOUT \
            || fdm12.state > STATE_AUTO) {
                // 1 可读，2 可写时推 fsm12
                fsm_driver(&fsm12);
            }
        if (ev.data.fd == fd2 && ev.events & EPOLLIN \
            || ev.data.fd == fd1 && ev.events & EPOLLOUT \
            || fsm21.state > STATE_AUTO) {
                // 2 可读，1 可写时推 fsm21
                fsm_driver(&fsm21);
            }
    }

    // 还原状态
    fcntl(fd1, F_SETFL, fd1_save);
    fcntl(fd2, F_SETFL, fd2_save);

    close(epfd);
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
