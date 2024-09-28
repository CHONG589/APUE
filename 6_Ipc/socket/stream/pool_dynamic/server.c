#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <signal.h>
#include <sys/mman.h>
#include <errno.h>
#include <time.h>

#include "proto.h"

#define SIG_NOTIFY      SIGUSR2 // SIGUSR1 和 SIGUSR2 是系统预留给我们自己定义行为的信号 

// 当空闲个数超过最大值时，就杀掉多余的空闲进程，当空闲个数少于
// 最小值时，则继续 fork。
#define MINSPARESERVER          5// 最少的空闲个数
#define MAXSPARESERVER          10// 最多的空闲个数
// 空闲的加上忙碌的不能超过 20 个。
#define MAXCLIENTS              20// 最多同时支持 20 个 client 访问
#define LINEBUFSIZE             80 
#define IPSTRSIZE               40 

enum {
    STATE_IDLE = 0,// 空闲
    STATE_BUSY
};

struct server_st {
    pid_t pid;// pid 为 -1，表示没有这样一个 server
    int state;
};

static struct server_st *serverpool;
static int idle_count = 0, busy_count = 0;
static int sd;

// 这个信号处理函数不需要做什么，只需要通知父进程就行，让父进程
// 识别到这个信号就行，然后父进程就执行后续操作。
static void usr2_handler(int s) {

    return ;
}

static void server_job(int pos) {
    
    int ppid;
    struct sockaddr_in raddr;
    socklen_t raddr_len;
    int client_sd;
    int len;
    long long int stamp;
    char linebuf[LINEBUFSIZE];
    char ipstr[IPSTRSIZE];

    ppid = getppid();

    while(1) {

        // 接受连接之前首先要通知自己的父进程，确认自己的状态
        serverpool[pos].state = STATE_IDLE;
        kill(ppid, SIG_NOTIFY);

        raddr_len = sizeof(raddr);
        client_sd = accept(sd, (void *)&raddr, &raddr_len);
        if (client_sd < 0) {
            if (errno != EINTR || errno != EAGAIN) {
                perror("accept()");
                exit(1);
            }
        }
        serverpool[pos].state = STATE_BUSY;
        // 改变完状态后要通知父进程来查看自己的状态。
        kill(ppid, SIG_NOTIFY);

        // 打印相关信息。
        inet_ntop(AF_INET, &raddr.sin_addr, ipstr, IPSTRSIZE);
        // printf("[%d] client: %s: %d\n", getpid(), ipstr, ntohs(raddr.sin_port));

        stamp = time(NULL);
        len = snprintf(linebuf, LINEBUFSIZE, FMT_STAMP, stamp);
        send(client_sd, linebuf, len, 0);
        sleep(5);
        close(client_sd);
    }
}

static int add_1_server(void) {

    int slot;
    pid_t pid;

    if (idle_count + busy_count >= MAXCLIENTS) {
        return -1;
    }
    for (slot = 0; slot < MAXCLIENTS; slot++) {
        if (serverpool[slot].pid == -1)
            break;
    }
    serverpool[slot].state = STATE_IDLE;
    // 注意不能直接用 serverpool[slot].pid 来接收，因为它有可能是失败的，
    // 这时是不需要的，而且如果是子进程时，是返回的 0，不是进程 pid ，所
    // 以不能这样，只有当是父进程时，返回值才是真正的子进程的 pid ，所以
    // 只能在父进程时将 pid 赋值给结构体中的 pid ， 
    pid = fork();
    if (pid < 0) {
        perror("fork()");
        exit(1);
    }
    if (pid == 0) {
        server_job(slot);
        exit(0);
    }
    else {
        // 父进程负责将子进程的 pid 填到结构体中，并且
        // 递增空闲状态的计数。
        serverpool[slot].pid = pid;
        idle_count++;
    }
    return 0;
}

static int del_1_server(void) {

    int i;

    if (idle_count == 0) return -1;

    for (i = 0; i < MAXCLIENTS; i++) {
        if (serverpool[i].pid != -1 && serverpool[i].state == STATE_IDLE) {
            // SIGTERM 信号用于终止进程，并执行清理任务
            kill(serverpool[i].pid, SIGTERM);
            serverpool[i].pid = -1;
            idle_count--;
            break;
        }
    }
    return 0;
}

// 对于在 add_1_server 和 del_1_server 中执行了 idle_count 等的加加操作，
// 这里又要执行该操作，是不是重复了？其实没有，前面两个是在刚加入的时候本身
// 就是要对空闲状态改变的，刚加入不就是变成空闲状态吗？所以要加加，这里是在
// 工作过程中，空闲和忙碌状态也会不断的改变，所以会更改，不冲突。
static int scan_pool(void) {

    int i;
    int busy = 0, idle = 0;

    for(i = 0; i < MAXCLIENTS; i++) {
        if (serverpool[i].pid == -1) {
            continue;
        }
        // 参数为 0 表示检测当前进程存不存在。
        if (kill(serverpool[i].pid, 0)) {
            // 不存在
            serverpool[i].pid = -1;
            continue;
        }
        if(serverpool[i].state == STATE_IDLE) {
            idle++;
        }
        else if (serverpool[i].state == STATE_BUSY)
            busy++;
        else {
            fprintf(stderr, "Unknown state.\n");
            // _exit(1);
            abort();
        }
    }
    // 先用自己的变量计数，最后再赋值给这个全局变量，这样
    // 产生冲突几率较小，因为频繁改变这个全局变量，有可能
    // 其它进程也会改变，冲突几率较大。
    idle_count = idle;
    busy_count = busy;
    return 0;
}

int main() {

    struct sigaction sa, osa;
    int i;
    struct sockaddr_in laddr;
    sigset_t set, oset;

    // sigaction 函数的功能是检查或修改与指定信号相关联的处理动作（可同时两种操作）
    
    // 当子进程结束的时候通过SIGCHLD信号告诉父进程，然后父进程再去释放其资源，
    // 如果没有收到该信号也不影响父进程的运行。
    // 以下三个条件中才会向父进程发送SIGCHLD信号:
    // 1.子进程终止时，2.子进程接收到SIGSTOP信号停止时，3. 子进程处在停止态，
    // 接受到SIGCONT后唤醒时。
    // 这里将 SIGCHLD 通知父进程收尸的操作取消掉,因为我们不进行收尸。
    
    // 修改 SIGCHLD 信号
    sa.sa_handler = SIG_IGN;
    sigemptyset(&sa.sa_mask);
    // 阻止子进程变成僵尸状态
    sa.sa_flags = SA_NOCLDWAIT;
    sigaction(SIGCHLD, &sa, &osa);

    // 定义 SIG_NOTIFY 信号
    sa.sa_handler = usr2_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    // 定义 SIG_NOTIFY 信号行为，用于通知父进程扫面 server 内
    // 的状态。每次改变了状态后就要用 Kill 发信号给父进程。
    sigaction(SIG_NOTIFY, &sa, &osa);

    // 光是定义 SIG_NOTIFU 信号可不行，因为涉及到并发，所以会被
    // 打断，所以还要将后面来的相同信号先阻塞在阻塞队列中。
    // 当 SIG_NOTITYP 信号的处理函数正在执行时，我不希望被
    // 打断，将后面来的 SIG_NOTIFY 信号 block 住。
    // 有时候不希望在接到信号时就立即停止当前执行，去处理
    // 信号，同时也不希望忽略该信号，而是延时一段时间去调用
    // 信号处理函数。这种情况是通过阻塞信号实现的。
    sigemptyset(&set);
    sigaddset(&set, SIG_NOTIFY);
    sigprocmask(SIG_BLOCK, &set, &oset);

    // 20 个结构体的空间
    serverpool = mmap(NULL, sizeof(struct server_st) * MAXCLIENTS, \
                    PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    if (serverpool == MAP_FAILED) {
        perror("mmap()");
        exit(1);
    }

    for(i = 0; i < MAXCLIENTS; i++) {
        serverpool[i].pid = -1;
    }

    sd = socket(AF_INET, SOCK_STREAM, 0);
    if (sd < 0) {
        perror("socket()");
        exit(1);
    }

    int val = 1;
    if (setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val)) < 0) {
        perror("setsockopt()");
        exit(1);
    }

    laddr.sin_family = AF_INET;
    laddr.sin_port = htons(atoi(SERVERPORT));
    inet_pton(AF_INET, "0.0.0.0", &laddr.sin_addr);
    if (bind(sd, (void *)&laddr, sizeof(laddr)) < 0) {
        perror("bind()");
        exit(1);
    }

    if (listen(sd, 100) < 0) {
        perror("listen()");
        exit(1);
    }

    // 初始时要有五个 server 端
    // 当 i 小于最小空闲进程数时，就要 fork。
    for (i = 0; i < MINSPARESERVER; i++) {

        add_1_server();
    }

    // 信号驱动程序，当进程池中有进程的状态发生改变，则发一个信号通知去看一下状态。
    // 即下面这段循环本来是一直循环扫描看状态的，加了 sigsuspend 后，只有当有信号通知
    // 了后才唤醒起来查看，没有则进入睡眠。
    // sigsuspend 相当于 sigprocmask 和 pause 的结合，sigprocmask 在上面有用到，而 
    // oset 是上面用 sigprocmask 前的状态，现在收到了信号要先解除阻塞，唤醒起来执行
    // 这后面的程序，即查询池。
    while(1) {

        // 由于可能有很多 SIG_NOTIFY 到来，导致前一个还没执行完，就被后面的打断，直接
        // 去执行后一个，所以用了 sigprocmask 把后面的阻塞住，而这里就是要唤醒阻塞队列
        // 中的进程。
        sigsuspend(&oset);
        // 遍历池内的状态
        scan_pool();

        // contrl the pool
        if (idle_count > MAXSPARESERVER) {
            for (i = 0; i < (idle_count - MAXSPARESERVER); i++) {
                del_1_server();
            }
        }
        else if (idle_count < MINSPARESERVER) {
            for (i = 0; i < (MINSPARESERVER - idle_count); i++) {
                add_1_server();
            }
        }

        // 输出当前池的状态
        for (i = 0; i < MAXCLIENTS; i++) {
            if (serverpool[i].pid == -1) {
                putchar(' ');
            }
            else if (serverpool[i].state == STATE_IDLE) {
                putchar('.');
            }
            else {
                putchar('x');
            }
        }
        putchar('\n');
    }

    // 恢复之前的状态
    sigprocmask(SIG_SETMASK, &oset, NULL);

    exit(0);
}
