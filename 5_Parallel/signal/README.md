异步事件的处理： 查询法（发生频率高）、通知法（发生频率低）

### 一、 信号

#### 1. 信号的概念

信号是软件中断， 信号的响应依赖于中断。

kill -l 查看所有的信号

标准信号值 1-34 

#### 2. signal();

```C
// 定义了一个函数指针类型
typedef void (*sighandler_t)(int); 

//当 signum 信号来了后，执行 handler 行为，最后返回之前的行为。
sighandler_t signal(int signum, sighandler_t handler); 

//如何表示一个函数返回一个函数指针
//前面的void (*func) (int)即表示一个函数指针
void (*func) (int) (int) 
void (*signal(int signum, void (*function)(int))) (int);

```

handler 可以是 SIG_IGN，SIG_DFL 或者函数的入口地址，信号会打断阻塞的系统调用（如 sleep，open，read）。

当为 SIG_IGN 时，即使这个信号来了，程序会忽略这个信号，继续执行程序。当然自己可以自定义 handler 函数的行为。

程序见 star.c，这个程序有 bug ，即在一直按住 Ctrl^C 的时候，程序执行没有十秒，而是 **快速执行完**。这是因为信号会打断阻塞的系统调用，程序中用 sleep(1) 阻塞程序，如果信号来了后，会打断它，令它不执行 sleep 这个系统调用。

要想解决不打断阻塞的系统调用，则可以用关于信号集的函数，即将对应的信号在你想要不能打断的程序片段前加入 sigprocmask() 将该信号 block ，然后在该程序片段末尾再次设置成 unblokc，这样即使在执行该片段之间来了该信号，但是我不响应，等执行完后再来响应。 

如我们之前写的程序中，在用 open 系统调用的时候，我们有判错的机制，但是那样写不严谨，因为这个错误也可能是因为一个信号打断它导致出错，正确的做法应该是如果为信号打断出错，则应该再进行打开。

```C
do {
	fd = open(argv[1], O_RDONLY);
	if (fd < 0) {
		if (errno != EINTR) {
			perror("open()");
			exit(1);
		}
	}
} while (fd < 0);
// 当不是信号打断出错时才报错。
```

其它系统调用也类似。

#### 3. 信号的不可靠

是指信号的 **行为** 不可靠，第一个信号调用还未结束，第二个信号又到达，两个连续的信号到达，后一个可能会把前一个覆盖掉，这是当两个信号相同时的情况。

#### 4. 可重入函数（解决信号不可靠）

[重入与不可重入函数](https://www.51cto.com/article/627877.html)

**该函数在执行过程中，再次调用该函数不会有影响，如一个进程中只能有一个 alarm() 函数。** 所有的系统调用都是可重入的，一部分库函数也是可重入的（如 memcpy ），不可重入的库函数如 localtime 等。

带有重入版本的库函数以 `_r` 结尾，不可重入的库函数不能作为信号处理函数，信号处理函数尽可能小，可防止可重入，响应函数不能使用公用的内容，防止资源冲突，如文件打开写入，因为他们公用了文件缓冲区。

#### 5. 信号的响应过程图

这里讲的是 **标准信号**，不是实时信号的讲解。

每个进程都要维护下面两个位图：

- mask：信号屏蔽字，表示当前信号的状态，一般值都为 1，即初始时全部信号都不屏蔽，如果你想忽略掉哪个信号，就把对应位置为 0，这样即使你收到了信号，按位与后也不会响应。
- pending：记录当前这个进程收到哪些信号，初值为 0，即没有信号，信号来了后，对应的信号位就会置为 1。

为什么信号从收到到响应有一个不可避免的延迟 ？

答：因为进程收到一个信号后，你是不知道的，你要怎么样才能知道呢？要等到你被中断打断后你才会知道，中断后，保存现场压栈，然后当前进程排队等待被调度，当调度到你的时候，即将从内核态变成用户态的时候，要做一个表达式的运算：mask & pending ，要是有信号的话，对应位按位与后是 1，然后执行对应的行为 handler。

经过分析，要知道有没有信号来，只有在被中断打断后，即将被调度的时候才会查看有没有信号，如果没有被中断，就永远不会知道有没有信号。**信号依赖于中断机制响应。**

在响应的时候，即执行行为函数的时候要将对应的 mask 和 pending 位都置为 0，响应回来后 mask 置为 1，pending 还是为 0，最后再做一次按位与（响应回来后会重新回到内核态恢复现场，又会从内核态到用户态，所以又会按位与操作），如果没有信号了，就走了。

思考：

- 如何忽略掉一个信号？ 

答：将 mask 中对应的信号置为 0。

- 标准信号为什么会丢失？

答：在当一个信号来了后，然后去响应它，在响应的过程中 mask 和 pending 位都为 0，这时又来了多个相同的信号，会把 pending 位置为 1，但是无论这里又多来了多少个信号，就只是为 1，并不会累加，下次扫描后，只会执行一次，所以在这个过程中，无论来了多少个相同的信号，最后只会执行一次，其它重复的会被丢失。

- 标准信号的响应没有严格的顺序 

答：根据优先级等机制决定。

- 不能从信号处理函数中随意的往外跳。（setjmp，longjmp）

答：因为你在执行信号处理函数前一步的时候，是要将 mask 和 pending 都设置成 0 的，等处理完后再将 mask 置为 1，如果你的信号处理函数中有 longjmp 类似的往外跳函数，这时就有可能错过了回到内核中重新将 mask 置为 1 的时机，导致这个进程以后都看不到该信号。解决方法：有专门的标准库函数：

```C
// 在跳之前保存旧的 mask
sigsetjmp();
siglongjmp();
```

#### 6. 常用函数

##### kill() 

是发信号的一个函数，并不只是杀掉进程。

`int kill(pid_t pid, int sig);` 

pid 值：

- pid 为正，给具体的某个进程发信号
- pid 为 0， 给同组的进程发送信号（组内广播）
- pid 为 -1， 除了 init 进程外，发送给当前进程有权限发送信号的每一个进程（全局广播）。
-  pid 小于-1，发给 **进程组号** 为其绝对值的进程

sig 信号值：

sig 为 0 时，不会发信号，但会检查进程或进程组是否存在，即检错。成功返回 0，失败返回 -1，不存在时返回值为 -1 且 error 会报 ESRCH，如果该进程存在但没有权限 error 为 EPERM。

##### raise() 

给当前进程发送信号，即自己给自己发送。

`int raise(int sig);` 

给进程自身发等同于 `kill(getpid(), sig);`，给线程自身发等同于`pthread_kill(pthread_self(), sig);` ，成功为 0，失败非 0

##### alarm(); 

只能精确到秒，以秒为单位的计时。

`unsigned int alarm(unsigned int seconds);` 

**alarm 函数是不可重入的，即当程序中有多个 alarm 时，前面的会被后面的覆盖，只执行最后一个。**

**倒计时为 0 时发送 SIGALRM 信号给自身进程**，SIGALRM 信号默认杀掉当前进程。seconds 为 0， 不会产生时钟信号。利用这个结合 signal 可以写一个定时程序。见 5sec_sig.c。

**注意：**

1. 每个进程只能设置一个 alarm 闹钟。
2. alarm 闹钟并不能循环触发，只能触发一次，若要实现循环触发，可以在 SIGALRM 信号处理函数中再次调用 alarm() 函数的设置定时器。
3. 如果我们想捕获 SIGALRM 信号，则必须在调用 alarm 之前安装该信号的处理程序，即 `signal(SIGALRM, alrm_handler);` 。

如果只用 alarm 定时，达到指定时间后直接结束了当前进程，并不会执行定时的任务。但可以结合 signal 函数做到，因为定时任务到时，会发送信号 SIGALRM 给自身进程，所以在程序中可以写 signal 函数来检测是否有信号来，如果信号来了，就执行对应的任务，这样就可以解决定时任务。

**疑问：** 像下面 pause 中的例子中用 while(1) 死循环后，定时到了后，会自动结束程序，但是下面写的例子中，五秒后会执行对应得任务，但程序好像没有结束？

```C
static int64_t count = 0;
static void alrm_handler(int s) {
    printf("%ld\n", count);
}

int main() {
	
    alarm(5);
    signal(SIGALRM, alrm_handler);
	
    while(1) {
        count++;
    }
	
    exit(0);
}
```

即时间到了后执行对应的行为，即打印 count ，但是它会打印，不会结束程序，不是时间到了后会自动杀死程序吗？

**原因：** 上面也写到了，定时到了后会发送 SIGALRM 信号，它默认是杀死当前进程的，就是你没有定义信号的行为时，才是杀死进程，但是上面例子中，你定义了行为函数，所以就不会执行默认行为，即不会杀死进程。所以会出现上面那种情况。

正确的写法：

```C
static int loop = 1;
static void alrm_handler(int s) {
    loop = 0;
}

int main() {
	
    int64_t count = 0;
	
    alarm(5);
    signal(SIGALRM, alrm_handler);
	
    while(loop) {
        count++;
    }
	
    // 定时结束后打印 count
    printf("%ld\n", count);
	
    exit(0);
}
```

这样定时到了后不会杀死进程，而是正常结束。

**例子：**

使用单一计时器，构造一组函数，实现任意数量的计时器。如之前使用 alarm() 函数时，在程序内使用多个 alarm() 时，只有最后一个会有用，所以使用单一计时器时就会有一个问题，多个一起使用时，只有最后一个会起作用。

##### pause() 

可以使得进程暂停运行、进入休眠状态，直到进程捕获一个信号为止，**只有执行了信号处理函数并返回**，pause 才会返回 -1，且 errno 置 EINTR 。

**注意：** **只有当一个信号递达且处理方式被捕获时， pause 函数引起的挂起操作才会被唤醒**，然后继续执行后面的程序。如果信号的处理方式为默认处理方式或者忽略，那么 pause 函数不会唤醒，而是一直挂起。

功能是等待一个信号来打断它。它不是忙等的，如下例子解释：

```C
int main() {

	alarm(5);       // 定时五秒，收到 SIGALRM 信号后，执行默认处理函数，即杀死进程。
	while (1);      // 阻塞，防止程序直接运行到最后 exit(0);
	
	exit(0);
}
```

这程序是忙等的，即运行这程序期间，cpu 处于忙碌状态。

```C
int main() {
	
	alarm(5);       // 定时五秒
	while (1)       // 阻塞，防止程序直接运行到最后 exit(0);
		pause();    // 将进程挂起，直到进程收到定时器信号，
		
	exit(0);
}
```

改成这样后，程序就不是忙等了，但功能是一样的。

**有些环境的 sleep 是由 alarm + pause 封装的，那么程序中用到了 sleep 语句，并且其它地方用到了 alarm 函数，那么程序就不能正常运行了，因为有多个 alarm 执行会出错，上面 alarm 中有解释。所以为了考虑可移植性，尽量不要用 sleep。**

###### 例子： signal() 、 pause() 和 alarm 实现定时的程序

**漏桶的实现：** 即在自己实现的 mycat 程序的基础上，让它不要一次性全部显示，如让它一秒显示 10 个字符，类似这种机制常用于播放器等中。程序见 slowcat.c 。

**令牌桶的实现：** 上面的漏桶中，如果此时没有数据，但是有定时在那里，一秒就会发送一个信号，然后使 pause 返回 -1，然后将 errno 置为 EINTR，读数据失败（没有数据可读），这样就一直 continue ，所以闲的时候一直在那空等，我们希望在闲的时候可以实现权限的积攒，如中间有三秒的时候没有数据，即空等了三秒，那么第四秒有数据的时候可以一次传输 30 个字符，这就是权限的积攒，更加灵活。程序见 slowcat2.c 。

slowcat2.c 程序解析：

```C
// 阻塞部分的代码
while (token <= 0) 
	pause();
token--;

// 信号处理函数代码
static void alrm_handler(int s) {
	alarm(1);
	token++;
	if (token > BURST) 
		token = BURST;
}

// 读取数据的代码
while ((readnum = read(src_fd, buf, BUFSIZE)) < 0) {
	if (errno == EINTR) continue;
	perror("read()");
	break;
}
```

当没有数据时，则会先等，一直在读数据那里 continue，由于有定时器 alarm(1)，则每秒会运行行为处理函数，即 token++，闲等了几秒，就加几次，这样就相当于积累权限。此时 token 是不会减减的，因为程序一直卡在读取数据那个循环，而信号处理函数是异步执行的。

当有数据后，由于积累了权限，积累了 3s，这时不会阻塞，token == 3 > 0，此时会在一瞬间处理 30 个字符，而不会阻塞，直到 token 减到 0 后，处理完了 30 个字符(不到一秒的时间处理 30 个字符，所以期间不会 token++)，继续阻塞，恢复到正常速度。

**封装成库的实现：** mytbf 文件中

除了那四个操作（函数）之外，真正的难点在于在哪里实现 signal() 和 alarm()；定义在 init 函数中。

- 当程序中需要两个不同速率的令牌桶时，那么就要两次 init 函数，这就会出现两个 alarm() 函数的情况，使程序出 bug。

答：定义一个全局标志 inited，使之在第一次 init 时才调用 signal() 和 alarm()。

- 将 signal() 和 alarm() 封装成 module_load() 函数。

- 有 module_load 那么就要有 module_unload，它的作用是如果程序在某一个位置异常结束，那么那些还没来得及释放的令牌桶要释放掉。那么何时来调用 unload 呢？不能在 destroy 中调用，因为destroy 是销毁一个速率的令牌桶，而 unload 是要销毁全部的令牌桶，显然不对。使用钩子函数，在 module_load 时注册钩子函数，将 module_unload 注册上去，钩子函数的参数就是 module_unload，即在销毁时使用 module_unload 来销毁。

钩子函数：一个进程在 **正常终止之前**，会调用钩子函数去释放所有该释放的内容。

**注意：** 要有宏观编程意识，把自己写的这个程序当成一个模块，进来时的状态是怎么样的，出来后就要还原回去，如你程序中使用了 alarm ，但是其他人不一定要用，他要用当然也是他自己会申请，不需要你的，你申请了，就释放回去，即恢复进来时的状态。

**例子：** 使用单一计时器实现多任务计时器。这里肯定是不能用到多个 alarm() 的，因为多个 alarm() 在同一个程序中使用会出错，所以只能使用 alarm() 来实现，那怎么来实现的，答案就是跟前面的令牌桶一样，使用一个计时器，假如为 alarm(1)，那么每过一秒，就统一为任务队列中存的倒计时减一，而不是为每一个任务分配一个时钟，而是一个时钟来管理，任务中的计时数存到自己空间中。程序在 anytimer 文件夹中。

**BUG :** 在程序 slowcat2.c 中有一点小问题，有同学会觉得在这里会有问题，这两句不是原子操作，会被打断。即执行完 while (token <= 0) 后，被信号打断，然后执行信号处理函数，token++，这时 token == 1，已经不满足 while 的条件了，所以不应该再继续等待，而由于不原子，所以会造成继续等待。其实这样分析是对的。

但是这是一个小问题，不太影响程序，因为即使是这样导致等待，也只是等了一秒，即在进程挂起后，下一次信号来了唤醒进程再判断的时候判断 token 已经为 2，自然也不会等待，这中间就差了一秒，最多就多读写了一次，所以不影响程序。因为多读写了一次不影响程序每秒只显示十个字符的整体性。

```C
while (token <= 0)
	pause();
```

**其实真正的问题是由 token--  产生的，因为它在有些机器上未必是一条指令完成的，有可能有多条指令完成，所以如果有多个任务都是使用相同的 token ，那么在做这些指令过程中被信号打断，就是出现 token 覆盖的结果，导致程序出错。**

解决：更改 token 的变量类型。保证 token-- 的原子性。

```C
static volatile sig_atomic_t token = 0;
```

##### setitimer()

可以替代 alarm() 函数，它不仅可以实现更加精确的计时，而且可以有更多的时钟选择方式。以后写程序建议使用这个函数，而不是 alarm() 。

setitimer() 还有一个好处是它会重复触发，即定时到了后，不需要像 alarm() 一样要重新设置，setitimer() 在定时到了后，会自动循环下去。

```C
int setitimer(int which, const struct itimerval *new_value,
            struct itimerval *old_value);
   
// which 表示选择时钟类型
// ITIMER_REAL 自然定时法则，就和 alarm 中的一样;对应的信号为（SIGALRM）
// ITIMER_VIRTUAL 只计算用户区代码运行的时间；对应的信号为（SIGVTALRM）
// ITIMER_FROF 计算用户区加上内核区的代码运行时间，不包含损耗时间；对应的信号为（SIGPROF）

// new_value 是你要设置的计时设置，它是一个结构体
struct itimerval {
	//周期性计时器的间隔
	struct timeval it_interval; 
	//距离下一次到期的时间
	struct timeval it_value;    
};

// it_value 指定初始定时时间，如果只指定 it_value，就是实现一次定时；如果同时指定 
// it_interval，则超时后，系统会将 it_interval 的值赋给 it_value，实现重复定时。
// 如果两者都是 0，那么就代表清楚定时器。

struct timeval {
	time_t      tv_sec;         /* seconds 秒*/
	suseconds_t tv_usec;        /* microseconds 微秒*/
};
// old_value 是保存旧的时钟
```

##### abort();

给当前进程发送一个 SIGABRT 信号，目的是结束当前进程，人为制造一个异常，得到出动现场。

##### system(); 

之前提到， 可以看作是 fork()，execl()，wait() 函数的封装，重提此函数，在有信号的程序中使用该函数时，需要 block 一个信号，ignore 两个信号

##### sleep(); 

它的问题，之前提到过。可以用 nanosleep、usleep 替换,，select 的 timeout 也可以实现休眠。

#### 7. 信号集

信号集类型： sigset_t

```C
// 清空信号集
int sigemptyset(sigset_t *set);

int sigfillset(sigset_t *set);

int sigaddset(sigset_t *set, int signum);

int sigdelset(sigset_t *set, int signum);

int sigismember(const sigset_t *set, int signum);
```

#### 8. 信号屏蔽字的处理

```C
// examine and change blocked signals 对某个信号集的信号做操作

int sigprocmask(int how, const sigset_t *set, sigset_t *oldset);

how：表示操作类型，有三种：
	- SIG_BLOCK：将 set 集合中信号阻塞，总的阻塞信号集将是 set 参数和当前环
		境的组合（可以理解为将 set 加入当前环境的 block 集）

	- SIG_UNBLOCK：将 set 中的信号从当前 block 集中删除。

	- SIG_SETMASK：设置 mask 位图为 set 的状态，set 可能是你之前保存的状态
		执行完你的程序出来后，现在要还原之前即 set 的状态。

这个函数就是系统给你的一种方式去控制上面讲到的信号响应过程中的 mask 位图。
我们不能决定信号什么时候来，但信号什么时候处理，我们是可以通过这个函数来
设置 mask 来达到目的。
信号虽然在代码段到达，但不响应，直到 SIG_UNBLOCK 后才会响应刚才到达的信号。
```

例如在 star.c 中，它是一直打印十个 `*` 的，当我们接收到 `Ctrl+c` 信号后，会打印一个 `!`，每接收一个信号就打印一个 `!`。由于 for 循环一直在那里循环，它是阻塞的，等打印完十个才不阻塞，通过这个程序说明 **信号可以打断阻塞的程序**。

所以我们在 block.c 中也是打印 `*` ，但信号来到后，我们先不打印 `！`，等到换行后再打印信号处理程序的 `！`，即在第二个 for 中的阻塞代码我们不能让信号打断，所以我们需要在执行这段代码前先将信号阻塞住，执行完后再 unblock 。但是由于 sigprocmask 函数是通过控制 mask 位图来控制信号什么时候来处理的，所以当多个信号来了后，不能累加信号的个数，所以只会执行第一个的处理程序。

因此一般情况下，在临界周围使用 sigprocmask，有两种写法：

1. 成对使用 SIG_BLOCK、SIG_UNBLOCK，意味着增加 set 中信号到 block 集和从 block 集删除 set 中信号。

```
sigemptyset(&set);
sigaddset(&set, SIGINT);
for(i = 0; i < 1000; i++) {	
	sigprocmask(SIG_BLOCK, &set, &oset);
	for(j = 0; j < 5; j++) {
		write(1, "*", 1);
		usleep(1000000);
	}
	write(1, "\n", 1);
	sigprocmask(SIG_SETMASK, &oset, NULL);
} 
```

但如果考虑模块化原则，使用前后环境状态并不发生改变，则这种写法不太稳妥，比如，当之前环境中的 block 集本身就有一个信号，那么 SIG_BLOCK 再增加此信号相当于无操作，但离开临界后使用 SIG_UNBLOCK ，则将此信号从 block 集删除，因此最后得到的 blcok 集和模块加载前不相同。

**注意**：虽然上面用 oset 保存了旧的状态，但它每次保存和恢复都是打印完一行之前保存这一行之前的状态，打印完后恢复旧的状态，如此循环，这个并不是该模块之前的状态，而是每行之前的状态。所以在整体程序之前还要保存一个就状态。

2. 正确做法：因为你不知道你现在要设置 block 的集合 set 中之前是不是 block 的，所以麻烦一点，先将 set 集合中的先统一设置为 unblock ，然后保存状态到 saveset 中，然后在 block 就可以不用保存状态了。

```
sigemptyset(&set);
sigaddset(&set, SIGINT);
sigprocmask(SIG_UNBLOCK, &set, &saveset);
for(i = 0; i < 1000; i++) {	
	sigprocmask(SIG_BLOCK, &set, NULL);
	for(j = 0; j < 5; j++) {
		write(1, "*", 1);
		usleep(1000000);
	}
	write(1, "\n", 1);
	sigprocmask(SIG_UNBLOCK, &set, NULL);  // unblock后还没走到
} 

sigprocmask(SIG_SETMASK, &saveset, NULL); 
```

程序看 susp.c。

#### 9. 扩展

##### sigsuspend()

```C
//wait for a signal， man 手册上的解释跟 pause() 一样，也就是信号来了后会落在 sigsuspend 中，多个来到时会阻塞在那，等前一个执行完再执行现在这个。

int sigsuspend(const sigset_t *mask);
```

**pause() 与 sigsuspend 的区别**

如在 block_2.c 中，先将 sigprocmask 函数的部分注释掉，即将原来是信号在一行中到来时，要等到第二行开头之前才会响应信号的处理函数，现在取消掉后，我们就什么时候收到信号，什么时候就响应。我们在每打印一行的后面加一个 pause()，即我们每打印完一行，就会停止打印，进入睡眠，除非此时有信号来领落到 pause 上，才会唤醒程序继续执行程序，此时既要执行信号处理函数，既要继续打印五个 * ，打印完一行后又等待。

这就是一个信号驱动程序，即信号来了后，就打印一行，然后暂停，再来了后，继续打印一行。

**注意**：由于将 sigprocmask 注释掉了，所以这里是可以打断阻塞的。

所以我们为了实现跟上面用 sigprocmask 一样，就要将该函数部分的注释部分加上去。如 susp.c 中 sigprocmask 和 pause 组合的部分，被注释掉了，注意这两个的顺序跟 block_2c 中的顺序已经不一样，要先恢复状态，再 pause 。

现在这个程序在打印一行的过程中即使再来一个信号，也不会在中间响应信号处理程序，而是在下一行前响应，但是虽然响应了第二个信号的处理函数，本来有了第二个信号，按道理会继续再打印一行，但是没有，只是响应了信号处理函数的部分。

这是为什么呢？这就要了解 pause 的概念了：**只有当一个信号递达且处理方式被捕获时， pause 函数引起的挂起操作才会被唤醒**。即确切的说是这样，有了信号之后，是信号处理函数砸到 pause() 上，而不是信号砸到 pause() 上，那么程序就有一个问题。

```C
sigprocmask(SIG_SETMASK, &oset, NULL);// 解除阻塞，使能看到信号。
pause();
```

在解除阻塞后的瞬间，信号处理函数就响应完了，并没有砸到 pause() 上，响应完后，再执行 pause() ，这样就不会唤醒程序了。**所以根本原因就是这两句是非原子的。**

sigsuspend 就能解决这个问题。它就相当于下面注释部分的操作，但是注释部分不能实现 sigsuspend 的效果，原因是不是原子操作。

```C
sigprocmask(SIG_UNBLOCK, &set, &saveset);
sigprocmask(SIG_BLOCK, &set, &oset);
for(i = 0; i < 1000; i++) {	
	
	for(j = 0; j < 5; j++) {
		write(1, "*", 1);
		usleep(1000000);
	}
	write(1, "\n", 1);
	sigsuspend(&oset);

	//sigset_t tmpset;
	//sigprocmask(SIG_SETMASK, &oset, &tmpset);  
	//pause();
	//sigprocmask(SIG_SETMASK, &tmpset, NULL);
} 
```

此程序在打印一行的过程中，即使有多个信号，也只是处理一个信号处理函数，且后面也只会打印一圈，并不会多少个信号就执行多少圈。即不能解决多个信号来时信号丢失的问题，因为标准信号一定会丢失，实时信号才不会。

**sigaction()**

signal() 使用上的缺陷，例如在 daemon 文件夹中的程序，程序中最后面的 fclose(fp) 和 closelog() 是执行不到的，因为我们的守护进程一定是异常终止的，由于守护进程的特点，脱离控制终端等特点，一直在后台运行，直到系统关机，我们需要用 kill 通过进程号来把它杀掉。

所以我们为了让能执行那两个关闭的代码，所以我们需要识别杀死进程的信号，即异常终止的信号，所以我们可以在程序中加上 signal。

```C
static void daemon_exit(int s) {
	// if (s == SIGINT) {
	//	  执行对应信号的操作
    // }
    // else if (s == SIGQUIT) {
	// 	  执行对应信号的操作
    // }
    // else if (s == SIGTERM) {
	//    执行对应信号的操作
    // }
	flcode(fp);
	closelog();
	exit(0);
}
signal(SIGINT, daemon_exit);
```

这样就可以执行关闭的代码了。其中信号处理函数中的参数 s 是用来识别信号的，即在用 signal 中识别不同的信号，然后处理对应的操作。但是当是这样时：

```C
signal(SIGINT, daemon_exit);
signal(SIGQUIT, daemon_exit);
signal(SIGTERM, daemon_exit);
```

第一次识别到 SIGINT 后，执行 daemon_exit 函数，当只执行了一句 close(fp 后释放 fp，然后被中断打断，即将从内核态回到用户态时，做按位与操作识别到 SIGQUIT 信号，然后再次执行 daemon_exit 函数释放，导致重复释放，即内存泄露。所以 signal 容易发生重入。

当多个信号来共用同一个信号处理函数的时候，我希望在响应某一个信号期间把其它的信号 block 住。能做到的就是 sigaction() 函数。**难道响应完再 unblock 后执行另一个信号的处理函数就不会？**

```C
// 对 signum 信号设置新的行为 act，并且保存旧的行为 oldact。
int sigaction(int signum, const struct sigaction *act,
            struct sigaction *oldact);

struct sigaction {
   void     (*sa_handler)(int);
   void     (*sa_sigaction)(int, siginfo_t *, void *);
   sigset_t   sa_mask;
   int        sa_flags;
   void     (*sa_restorer)(void);
};

// sa_handler此参数和signal()的参数handler相同，代表新的信号处理函数
// sa_mask 用来设置在处理该信号时暂时将 sa_mask 指定的信号集搁置
// sa_sigaction 是另一个信号处理函数，它有三个参数，可以获得关于信号的更详细的信息。
// 当 sa_flags 成员的值包含了 SA_SIGINFO 标志时，系统将使用 sa_sigaction 函数作为
// 信号处理函数，否则使用 sa_handler 作为信号处理函数。
```

程序详见 signal 文件中的 daemon 文件夹的 mydaemon.c

signal 的另外一个问题，如在我们实现的 mytbf 程序中，实现的时流量的控制，即 slowcat2.c 中的内容，当我们执行 make 的时候，然后通过另一个终端进行 `while true; do kill -ALRM 15148 ; done` 时，15148 表示 mytbf 进程，持续向该进程发送 ALRM 信号，然后就会发现 mytbf 进程的流量控制失效了，不是一秒一秒的显示，而是直接显示完。程序出现了 BUG。

signal 并不会区分信号的来源，直接执行信号行为。而显然我们程序中实现流量控制中的信号时从内核中来的，而终端中输入是用户发送的。sigaction 函数可以识别来源，就是那个三参数函数。

```C
// 第一个参数是信号处理函数，第二个参数是一个结构体，表示信号的
// 信息，里面的 si_code 可以识别信号的来源，第三个无
void     (*sa_sigaction)(int, siginfo_t *, void *);

siginfo_t {
   int      si_signo;     /* Signal number */
   int      si_errno;     /* An errno value */
   int      si_code;      /* Signal code */
   int      si_trapno;    /* Trap number that caused
							 hardware-generated signal
							 (unused on most architectures) */
   pid_t    si_pid;       /* Sending process ID */
   uid_t    si_uid;       /* Real user ID of sending process */
   int      si_status;    /* Exit value or signal */
   clock_t  si_utime;     /* User time consumed */
   clock_t  si_stime;     /* System time consumed */
   sigval_t si_value;     /* Signal value */
   int      si_int;       /* POSIXb signal */
   void    *si_ptr;       /* POSIXb signal */
   int      si_overrun;   /* Timer overrun count;
							 POSIXb timers */
   int      si_timerid;   /* Timer ID; POSIXb timers */
   void    *si_addr;      /* Memory location which caused fault */
   long     si_band;      /* Band event (was int in
							 glibc 2.3.2 and earlier) */
   int      si_fd;        /* File descriptor */
   short    si_addr_lsb;  /* Least significant bit of address
							 (since Linux 2.6.32) */
   void    *si_lower;     /* Lower bound when address violation
							 occurred (since Linux 3.19) */
   void    *si_upper;     /* Upper bound when address violation
							 occurred (since Linux 3.19) */
   int      si_pkey;      /* Protection key on PTE that caused
							 fault (since Linux 4.6) */
   void    *si_call_addr; /* Address of system call instruction
							 (since Linux 3.5) */
   int      si_syscall;   /* Number of attempted system call
							 (since Linux 3.5) */
   unsigned int si_arch;  /* Architecture of attempted system call
							 (since Linux 3.5) */
}
```

1）解决方法，响应当前信号时，block 其它信号。

2）可以通过 si_code 判断信号的来源，从而只响应从内核来的信号，将之前的 signal 都变为 sigaction 处理。

setitimer();  将会替换 alarm。程序在 mytbf_sa 文件夹中。可以看到执行终端上发的信号后，流控会变慢，大概两秒时间，这是因为添加了判断信号来源后，会打断程序运行的时间，即会空出一秒。

#### 10. 实时信号

之前讲的都是标准信号，会丢失，没有严格的响应顺序，因为用的是位图。如果同时收到两种信号，那么先响应标准信号，后响应实时信号。其中 susp_rt.c 程序解决了信号丢失的问题，即一下来了多个相同的信号，不是只响应一个，而是全部都会响应，且排队（能排队的数量 ulimit -a 中 pending sigals）。
