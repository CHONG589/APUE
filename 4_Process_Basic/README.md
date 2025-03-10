# 进程标识符 pid

- 类型 pid_t，传统上是 **有符号** 的 16 位整形数，如果 pid 用正数表示，则最多表示 2^15 - 1 = 32767。
- ps 命令 ：**打印出当前进程的情况**。用 ps 的常用组合：ps axf 和 ps axm。
- 进程号是顺次向下使用，和文件描述符 fd 使用规则不同（当前可用范围内最小的），哪怕进程销毁，下一要用的进程号也是当前加一，即使前面的进程号有空闲的，它不会使用前面的，而是继续往后面找。

```C 
// 获取当前进程号
pid_t getpid(void);

//获取当前进程的父进程号，这两函数总是成功
pid_t getppid(void);
```

# 进程的产生

## fork()

表示 create a child process 。

```C
#include <sys/types.h>
#include <unistd.h>

pid_t fork(void);
```

### DESCRIPION

> fork()  creates  a  new  process  by  **duplicating**  the calling process.  The new process is referred to as the child process.  The calling process is referred to as the parent process.

fork() 通过 **复制** 调用进程来创建一个新进程。新进程称为子进程。调用进程称为父进程。

> The  child process and the parent process run in separate memory spaces.  At the time of fork() both memory spaces have the same content.   Memory  writes,  file mappings (mmap(2)), and unmappings (munmap(2)) performed by one of the processes do not affect the other.

子进程和父进程在不同的内存空间中运行。在 fork() 时，两个内存空间的内容相同。其中一个进程执行的内存写入、文件映射 (mmap(2)) 和取消映射 (munmap(2)) 不会影响另一个进程。

 // 和 setjmp 一样，也是执行一次，返回两次。

注意理解关键字: duplicating，意味着拷贝复制 " 一模一样" 含义，创建一个子进程是通过复制父进程的方式来进行创建。

init 进程：1 号，是所有进程的祖先进程。

fork 后父子进程的区别：

- fork 返回值不同
- pid 不同，有各自的 pid
- ppid 不同，各自的父进程肯定也不同
- 未决信号和文件锁不继承
- 资源利用率清零

例如下面这段代码：

```C
int main () {

    pid_t pid;
    printf("[%d]:Begin!\n", getpid());

	// fflush(NULL);

    pid = fork();
    // fork 开始后，代码开始分裂为两份，即创建子进程。然后在父子进程当中各自负责
    // 判断 if (pid < 0) 这条判断语句。最后，子进程中会判断进入 if (pid == 0)
    // 这句，父进程会进入 else 里面进行执行。
    
    if (pid < 0) {
        perror("fork()");
        exit(1);
    }
    if (pid == 0) {
        // child
        printf("[%d]:Child is working!\n", getpid());
    }
    else {
        // parent
        printf("[%d]:Parent is working!\n", getpid());
    }

    printf("[%d]:End!\n", getpid());

    exit(0);
}

// 结果：

// [268540]:Begin!
// [268540]:Parent is working!
// [268540]:End!
// [268541]:Child is working!
// [268541]:End!

// 进程关系：
// 268540 pts/0    S+     0:00        \_ ./fork
// 268541 pts/0    S+     0:00            \_ ./fork

// 谁先调度由调度器的调度策略决定，所以父子进程不确定哪个先运行
```

### 程序会出现的问题

即当打印的字符不在终端输出，而是重定向输出到文件中时，如：

```C
// ./fork > /tmp/out
// cat /tmp/out

// 结果如下：
// [269020]:Begin!
// [269020]:Parent is working!
// [269020]:End!
// [269020]:Begin!
// [269021]:Child is working!
// [269021]:End!
```

发现出现了两个 Begin ，并且如果在 fork 之前的 printf 中的 \n 去掉，在终端输出也是一样，原因是在创建进程时(fork)，由于复制了一份一模一样的，所以父进程中的流也被复制过去了，复制之前缓冲区本来就有 Begin 了，所以在没加 \n 也会出现这个问题，加了后输出到终端就不会打印两个 Begin ，\n 刷新了缓冲区。

那么加了 \n 后输出到文件中为什么还会呢？因为终端是标准的输出设备，默认是行缓冲，所以加 \n 刷新了有用，而输出到文件当中默认是全缓冲模式，\n 就只是换行，只有当缓冲区满了才会输出，所以在还没满，即还没输出时，创建了进程，由于是采用全缓冲，没刷新，还留在缓冲区中，创建的进程缓冲区中也有一份同样的内容，等缓冲区满以后，就又会打印 Begin，而父进程中也会打印 Begin，这样就出现了两个 Begin。

在 fork 之前一定要 fflush，不然如果缓冲区中有数据，fork 之后会产生两个相同数据的缓冲区。

### 例题

求 30000000 到 30000200 之间的质数。

primer0.c 文件中的是采用传统方法做的，primer1.c 文件是采用 fork 来写的。

```C
//primer0.c

#define LEFT    30000000
#define RIGHT   30000200

int i, j, mark;

for(i = LEFT; i <= RIGHT; i++) {
	mark = 1;
	for (j = 2; j < i / 2; j++) {
		if (i % j == 0) {
			mark = 0;
			break;
		}
	}
	if (mark) {
		printf("%d is a primer\n", i);
	}
}
// real    0m0.664s
// user    0m0.658s
// sys     0m0.005s
```

注意用多进程来求每一个数时，每一个子进程做完该做的事情要 exit(0)，不然会无线创建进程，详见 primer1.c 。

```C
//primer1.c

/**
 * 用 201 个子进程来代替这 201 个计算任务，即每产生一个
 * i 值就扔给 fork 进行计算。
 */
int i, j, mark, pid;

for(i = LEFT; i <= RIGHT; i++) {
	pid = fork();
	if (pid < 0){
		perror("fork()");
		exit(1);
	}
	if (pid == 0) {
		// child
		mark = 1;
		for (j = 2; j < i / 2; j++) {
			if (i % j == 0) {
				mark = 0;
				break;
			}
		}
		if (mark) {
			printf("%d is a primer\n", i);
		}
		//sleep(1000);
		exit(0);
	}
}
// real    0m0.039s
// user    0m0.002s
// sys     0m0.024s

sleep(1000);
exit(0);
```

### init 进程

子进程由父进程产生 ( fork() )，也要符合谁创建谁释放的原则，既然由父进程创建，则必须由父进程进行其资源的回收。

例如拿 primer1.c 中的程序为例，在每个子进程 exit(0) 前，也就是退出前，sleep(1000)，让所有子进程延时后再退出，先让父进程退出，运行程序后，用 ps axf 查看进程信息，发现所有子进程并没有原来的父进程，而是 ./primer1 的形式，表示它的父进程是 init 进程，由于父进程创建了并没有释放子进程，所以 init 进程替它收尸子进程，所以属于 init 进程。

将 sleep(1000) 换到父进程 exit(0) 前，即让子进程先退出，父进程后退出，用 ps axf 查看后，各子进程的父进程是原来的父进程，只不过这些子进程变成了僵尸进程。因为子进程都退出了，它们的父进程还是原来的，而又没有释放掉，所以变成了僵尸进程，等父进程退出后，就会也被 init 进程收尸。

一般父进程释放子进程前一般都会 1. 查看子进程的状态，2. 释放 pid 号(非常宝贵)。

**僵尸态进程：** 未回收的进程， 不占用太多的内存，但占用进程号。

**孤儿进程：** 其父进程消亡，将被 init 进程接收，即父进程先执行结束退出，子进程就称为孤儿进程。

## vfork()

为了防止拷贝时将所有数据都复制所导致的浪费。但现在 fork 已经采用了写时拷贝，读则共享，谁要写就谁拷贝。

# 进程的消亡及释放资源

见 fork1 primer0 primer1 primer2

**wait()：** 等待进程状态发生变化，有几个宏来判断 status 的状态，此函数是没有指向的，即没有指定收谁的尸，只有收回来后检测 pid 才知道。这个函数是死等的。

**whitpid()：** 可以指定收哪个尸。options 参数可以实现非死等，即非阻塞。

`pid_t waitpid(pid_t pid, int *status, int options)`

options 具体使用查看 man。

其实按照 primer2.c 中的方法来解决问题也不是最好的，还是有 bug ，因为当中我们采用了 201 个进程来计算待计算 201 个数值，如果我们把数值范围扩大到很大，那岂不是要更多的进程，进程数是有限的，不可能给你这么多进程，更何况你就是写这么一个小小的程序。所以这样做明显是不合理的。

所以我们采用特定的进程数来计算，进程数 n 可以取 3 或者 4 等等，反正是确定的，n 个进程解决范围内的计算数值。

1. **分块：** 将范围内的数值个数除以 n ，就是每个进程要计算的数值个数。不是最优的方法，因为即使是分块，每个进程承担的计算压力不均匀，质数集中分布较多的那一块计算压力更大。

2. **交叉分配：** 跟发牌一样，按顺序发，发完一圈再重复从刚开始的那个人开始发。随机性还是不够，因为特定的进程一定是某个数的整数倍，还是有规律，但优于分块。程序在 primerN.c 中。

3. **池类算法：** 用一个进程负责将数值放入池中，另外三个进程负责计算数值，谁计算完，谁就从池子中抢来计算，即谁有空谁就去抢。涉及到抢占，进程间通信，暂时还写不出该程序。

# exec 函数族

可以看这篇文章 [exec族函数详解](https://blog.csdn.net/m0_69211839/article/details/129214699) 

execute a file. The  exec()  family  of  functions replaces the current process image with a new.

exec 函数族提供了一个在进程中 **启动另一个程序执行** 的方法。它可以根据指定的 **文件名** 或 **目录名** 找到可执行文件，并用它来取代原调用进程的数据段、代码段和堆栈段，在执行完之后，原调用进程的内容除了进程号外，其他全部被新的进程替换了。另外，这里的可执行文件既可以是二进制文件，也可以是 Linux 下任何可执行的脚本文件。

process image 用新的 process 替换当前的 process。

```C
int execl( const char *path, const char *arg, …);
int execlp( const char *file, const char *arg, …);
int execle( const char *path, const char *arg , …, char * const envp[]);
int execv( const char *path, char *const argv[]);
int execvp( const char *file, char *const argv[]);

// 前三个才是定参的实现，后三个才是变参的实现

// path：可执行文件的路径名字，如想在一个进程中调用执行 ls 命令，可以用 execl 函数，path 中
// 是填 ls 命令的二进制可执行文件的路径。
// arg：可执行程序所带的参数，第一个参数为可执行文件名字，没有带路径且 arg 必须以 NULL 结束。
// file：它不是用指定路径的方式找到可执行文件的，而是直接给出文件名，这种方式必须要有对应的环
// 境变量，这样你给出文件名才能搜索到，然后执行对应的可执行文件。
// p：使用文件名，并从 PATH 环境进行寻找可执行文件。
// v：应先构造一个指向各参数的指针数组，然后将该数组的地址作为这些函数的参数。
// e：多了 envp[] 数组，使用新的环境变量代替调用进程的环境变量。
```

目前类似 argv 的还有几处，如 glob 函数，environ 函数。如在终端中宏执行 data +%s 这条命令，使用 execl() 函数来实现。execl("/bin/date", "date", "+%s", NULL) 。

/bin/date 是要执行 date 命令的二进制文件， date 是命令名称，+%s 为格式参数，最后结束 NULL。

execl 函数如果成功创建进程，那么就会有新进程创建，进行 **代替** 它，代表着永远不会回来原进程了，如果失败，它会返回 -1 或 errorn，那么它就还在原进程中执行，即报错可以这样写。

```C
execl("/bin/date", "date", "+%s", NULL) ;
perror("execl()");
exit(1);
```

具体实现见 exec1.c 文件。但是此程序这样写还会有问题，如果在终端输出时，程序正常，即会打印 Begin 和时间戳，不会打印 End ，但是执行时输出重定向到 /tmp/out 时，发现 /tmp/out 中只有时间戳而没有 Begin 。

没有输出是因为 puts 输出是虽然有 '\n' ，这对终端中有用，但是在输出重定向到文件中时是没用的，当在文件中时，先 puts，但是还在缓冲区中，还没打印，但是紧接着你 execl 代替了原进程，并不是复制，新的缓冲区就是空的，不跟旧缓冲区一样，所以在新进程中不会输出 Begin。

所以要在 puts 后，execl 前先用 fflush 刷新，使之先在代替进程前输出 Begin，然后再 execl 创建代替的新进程，这样即使新进程缓冲区中没有，在输出文件中也会看得到，因为它在创建新进程前就已经输出了，而输出到终端中时因为使用的是行缓冲，用 '\n' 时就会刷新，也是先输出了缓冲区。

**注意：**

1. **替换后 pid 不变**。
2. exec 执行前要执行 fflush，否则缓冲区。

fork、exec、wait 三个函数的组合使用 见 exec2.c。

**shell 的外部命令实现见 myshell.c。（重点看！！！）**

# 用户权限及组权限（涉及到 unix 的身份切换）

s 选项指的是 Set-User-ID 和 Set-Group-ID。当文件或目录启用了 Set-User-ID 或 Set-Group-ID 特性时，**执行该文件或访问该目录的用户或组将自动继承该文件或目录的所有者和组**。简而言之，启用了 s 选项后，文件或目录将 **使用其所有者或组的权限** 而 **不是执行用户或组的权限**。

当文件启用 Set-User-ID 特性后 (即 u + s)，任何执行该文件的用户将自动继承该文件的所有者的权限。这意味着即使执行该文件的用户没有访问该文件的权限，他们也可以执行该文件，并以文件所有者的身份访问该文件。这种功能通常用于需要管理员特权的程序。

例如：/usr/bin/passwd 程序允许用户更改自己的密码。该程序只有在拥有 root 用户权限时才能修改 /etc/shadow 文件。为了让该程序在普通用户权限下正常工作，passwd 程序可以使用 Set-User-ID 特性，这样它将始终以 root 用户的身份运行，即使普通用户执行该程序也是如此。

u+s：某个可执行文件具有 u+s 权限，任何人在调用此可执行文件时，就切换为该可执行文件的 user 身份

g+s：某个可执行文件具有 g+s 权限，任何人在调用此可执行文件时，就切换为该可执行文件的 group 身份

**例题：**

编写程序完成类似于 `sudo root cat /etc/shadow` 的功能，含义是以 root 用户的身份查看 shadow 文件，即切换成 root 用户。我们只要编写成 `./mysu 0 cat /etc/shadow` 这样即可。

具体过程见 mysu.c 。

编写完成程序后执行并不能成功执行，因为如果这样你用 setuid 就能完成切换用户，那还需要 sudo 干嘛，我直接编写程序就完成了。

```
ls -l mysu
-rwxrwxr-x 1 zch zch 17040 7月  16 21:31 mysu
mysu 的所有者为 zch

我们要将程序的所有者改成 root
su -
chown root mysu
ls -l mysu
-rwxrwxr-x 1 root zch 17040 7月  16 21:31 mysu

设置 mysu 文件的 s 特性
chmod u+s mysu
ls -l mysu
-rwsrwxr-x 1 root zch 17040 7月  16 21:31 mysu
```

为什么要将 mysu 的所有者改为 root ？请看上面 s 的详细解析，**当文件或目录启用了 Set-User-ID 或 Set-Group-ID 特性时，执行该文件或访问该目录的用户或组将自动继承该文件或目录的所有者和组。** 所以当其它用户执行该文件时就会切换 (继承) 成 root 用户。

所以我们还要设置 mysu 的权限有 s 的特性。

# 观摩课：解释器文件

脚本文件如 shell、py 文件。

# system()

可以看作 fork、exec、wait 的封装，这个函数就是调用 shell 实现的。

# 进程会计 (统计进程所占用的资源量)

# 进程时间 (使用 time 命令)

times() 函数

# 守护进程

守护进程的详细讲解：[守护进程的基础概念](https://blog.csdn.net/weixin_49503250/article/details/130662154) 和 [守护进程的定义](https://blog.csdn.net/JMW1407/article/details/108412836)

## 会话 session，标识 sid

- 会话（session）是一个更高级别的抽象概念，用于表示一个或多个进程组的集合。
- 每个会话都有一个唯一的会话 ID（SID），与会话的领头进程（即创建会话的进程）的 PID 相同。
- 会话用于对一组相关进程组进行管理，例如在注销时关闭所有相关进程组。
- 一个会话可以有一个关联的控制终端，但也可以没有。
- 当一个会话有关联的控制终端时，该会话称为前台会话，而其他没有关联控制终端的会话称为后台会话。

会话是一个或多个进程组的集合。每个会话都有一个唯一的会话 ID，所有在同一个会话中的进程都共享该会话 ID。在一个会话中，**可以有多个进程组，但只有一个进程组可以成为前台进程组，其他的则为后台进程组**。同时，会话中也可以只有后台进程组，没有前台进程组。

## 进程组与会话之间的关系

**进程组和会话是两个层次的概念**：进程组是进程的组织方式，而会话是进程组的组织方式。换句话说，一个会话可以包含多个进程组，而一个进程组可以包含多个进程。会话和进程组的关系是为了更好地对进程进行组织和管理，以便对进程进行信号发送和资源控制等操作。

登录 Shell 为例：

当用户登录时，系统会启动一个 shell 进程，然后 shell 进程会创建一个 **新的会话** 与 **新的进程组**，并自己成为该会话的领头进程与该进程组的领头进程，所以 shell 进程的 pid，gid，sid 都是相同的，之后 shell 进程将我们的远程终端设置成为这个会话的控制终端。

这个新的进程组成为前台进程组，它可以接收来自控制终端的信号。之后，用户在 Shell 进程中运行的其他进程会继承会话的信息。

## 前台进程组、后台进程组

- 只有前台进程组，才会拥有控制终端，一个会话中最多只能有一个前台进程组，当然也可以没有前台进程组。
- 只有前台进程组中的进程才能从控制终端中读取输入。当用户在控制终端中输入终端字符生成信号后，该信号会被发送到前台进程组中的所有成员。

## 守护进程概念

守护进程（daemon）是在 Unix 和类 Unix（如 Linux）操作系统中运行的一种 **特殊的后台进程**，它们独立于控制终端并且周期性地执行某种任务或等待处理某些发生的事件。

- 守护进程通常在系统引导装载时启动，并且在系统关闭之前一直运行。守护进程的名称通常以 "d" 结尾，以便于区分。例如，sshd 是 Secure Shell 守护进程，httpd 是 HTTP 守护进程。
- 这些进程在后台运行，提供各种服务，例如处理网络请求（如 web 服务器）、处理系统日志、处理电子邮件和其他各种任务。
- 守护进程通常不直接与用户交互，但它们工作起来非常重要，为其他程序和用户提供关键服务。
- 守护进程在创建时通常会进行 “孤儿化” 操作，使其成为 init 进程（进程 ID 为1）的子进程。这样，它们就可以在后台运行，不受任何特定用户或会话的影响。此外，守护进程通常会更改其工作目录到根目录 (“/”)，关闭所有已打开的文件描述符（包括输入、输出和错误输出），并重新打开标准输入、标准输出和标准错误到 /dev/null，这样就可以防止它们不小心读取或写入任何用户文件或终端。

总的来说，守护进程是一种在后台运行，为系统或其他程序提供服务的进程。

清除进程的 umask 以确保当守护进程创建文件和目录时拥有所需的权限。umask 是一个权限掩码，它决定了新建文件或目录的默认权限。清除 umask 是为了让守护进程 **能够有更大的权限控制它创建的文件或目录**。

```C
pid_t setsid(void);     // creates a session and sets the process group ID

// creates a new session if the calling process is not a process group leader.
// group leader: 当父进程创建了子进程后，那么父进程就是 group leader，所以这句话就是
// 在说父进程不能调用 setsid，只能子进程调用。

// The calling process is the leader of  the new session.

// The calling process also becomes the process group leader of a new process 
// group in the session.

// the new session has no controlling terminal. 脱离控制终端
```

例：实现一个守护进程（不是一个完整的守护进程，会被杀死），守护进程执行的任务是持续向一个文件当中去输入数字，隔一秒递增输出到文件中。那么就不能用 argv 来指定文件，因为守护进程会脱离控制终端，所以不能用命令行来指定文件。否则就跟标准输入输出相关联了。见 mydaemon.c

# 系统日志

syslogd 服务, 所有需要写系统日志的操作将其交给 syslogd，实现权限分离。我们在输出内容的时候不要加上格式，如换行符，因为怎么安排，syslogd 会帮我们实现，加上 \n，内容中会出现 \n，而不是换行的意思。

**系统日志的实现：**

```C
openlog();        // 用来建立与系统日志的链接，获取对系统日志文件的操作权
syslog();         // 提交内容
closelog();       // 断开与 syslogd 的连接
```

这三个函数的讲解 [openlog函数](https://blog.csdn.net/weixin_41111116/article/details/120239768)

详细程序见 mydaemon.c ，写的日志在 /var/log/syslog 下查看，syslog 是系统的主日志。

[Ubuntu系统日志详解](https://geek-docs.com/ubuntu/ubuntu-ask-answer/170_tk_1704237588.html#:~:text=%E7%B3%BB%E7%BB%9F%E6%97%A5%E5%BF%97%E6%98%AFUbuntu%E4%B8%AD%E9%87%8D%E8%A6%81%E7%9A%84%E8%B5%84%E6%BA%90%EF%BC%8C%E7%94%A8%E4%BA%8E%E8%AF%8A%E6%96%AD%E9%97%AE%E9%A2%98%E3%80%81%E7%9B%91%E6%B5%8B%E7%B3%BB%E7%BB%9F%E7%8A%B6%E6%80%81%E5%92%8C%E5%88%86%E6%9E%90%E6%80%A7%E8%83%BD%E3%80%82,%E9%80%9A%E8%BF%87%E6%9F%A5%E7%9C%8B%E5%92%8C%E5%88%86%E6%9E%90%E7%B3%BB%E7%BB%9F%E6%97%A5%E5%BF%97%EF%BC%8C%E6%82%A8%E5%8F%AF%E4%BB%A5%E4%BA%86%E8%A7%A3%E7%B3%BB%E7%BB%9F%E6%B4%BB%E5%8A%A8%E3%80%81%E5%8F%91%E7%8E%B0%E6%BD%9C%E5%9C%A8%E9%97%AE%E9%A2%98%E5%B9%B6%E5%81%9A%E5%87%BA%E7%9B%B8%E5%BA%94%E7%9A%84%E8%B0%83%E6%95%B4%E3%80%82%20%E5%9C%A8%E6%9C%AC%E6%96%87%E4%B8%AD%EF%BC%8C%E6%88%91%E4%BB%AC%E4%BB%8B%E7%BB%8D%E4%BA%86%E7%B3%BB%E7%BB%9F%E6%97%A5%E5%BF%97%E7%9A%84%E7%9B%AE%E5%BD%95%E7%BB%93%E6%9E%84%E3%80%81%E6%9F%A5%E7%9C%8B%E6%96%B9%E6%B3%95%E3%80%81%E5%88%86%E6%9E%90%E6%8A%80%E5%B7%A7%E5%92%8C%E8%87%AA%E5%AE%9A%E4%B9%89%E9%80%89%E9%A1%B9%E3%80%82)

	
