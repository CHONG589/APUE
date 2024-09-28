### 二、线程

#### 1 线程的概念

是一个正在运行的函数

- posix 线程是一套标准，不是一个实现，即只要求你要完成什么功能，具体的实现不要求一样。

- openmp 线程也是一套标准。

线程标识： pthread_t，ps -axm 查看线程，ps ax -L 以 Linux 的形式查看线程

```C
// 比较线程标识
//Compile and link with -pthread.编译时要加上 -pthread
int pthread_equal(pthread_t t1, pthread_t t2); 

// 返回当前线程标识
//Compile and link with -pthread.编译时要加上 -pthread
pthread_t pthread_self(void); 
```

#### 2 线程的操作

##### （1）线程的创建

```C
// thread 用来保存创建线程后的线程标识
// attr 属性，80% 都可以用默认来解决，一般填 NULL
// start_routine 函数指针，就是你要求线程完成的任务（函数）
// arg，函数的参数，因为第三个参数存的是函数的入口地址，参数要另外存，多参数可以封装成结构体来
//      传。
int pthread_create(pthread_t *thread, const pthread_attr_t *attr, 
				   void *(*start_routine) (void *), void *arg);

// 成功返回0，失败返回 errno，不能用 perror() 来报错，只能用 strerror() 来报错。
```

线程的调度取决于调度器策略

##### （2）线程的终止 （3种方式）

1. 线程从启动例程返回，返回值就是线程的退出码(即线程函数中 return 返回值)。
2. 线程可以被 **同一进程中的其他线程取消** (如果此线程是进程中的最后一个线程，则终止时进程也结束)。
3. 线程调用 pthread_exit() 函数，如果此线程是进程中的最后一个线程，则终止时进程也结束，相比于用 return 返回，使用 pthread_exit() **可以实现线程栈的清理**。

进程中用 exit() 会调用钩子函数，那么在线程中调用 pthread_exit() 则可以实现线程栈的清理。

pthread_join()  --> 相当于进程的 wait()，使用了收尸后，不会像前面没收尸的时候出现线程还没创建完，程序就直接退出了，导致不到打印线程的函数，有了收尸后会等待创建完线程，所以线程一定会运行，然后进行收尸。

程序在 create1.c 中。

##### （3）栈的清理

```C
//这两个都是宏，必须得成对出现。

pthread_cleanup_push()      //相当于注册钩子函数
void pthread_cleanup_push(void (*routine)(void *),

pthread_cleanup_pop()       //相当于取钩子函数
//excute标识是否调用，决定是否执行。
void pthread_cleanup_pop(int execute); 
```

这两个函数放在线程函数中，在 pthread_exit() 退出前按注册顺序逆序调用。**必须成对出现，否则会报错。** 程序详见 cleanup.c。

##### （4）线程的取消选项

在多线程任务执行时，如果某一个线程找到了某一个东西，那么其它线程就没必要执行下去了，那么这时要让它们停下来，该怎么让其它线程停下来呢？收尸（pthread_join()）?但是一个线程正在运行是无法进行收尸的，只有停下来后才可以收尸，所以我们就要有一个线程取消的选项，先取消使其停下来后再进行收尸。

**线程取消：** `int pthread_cancel(pthread_t thread);`，取消有 2 种状态：允许取消和不允许取消。

允许取消又分为： 异步 cancel， 推迟 cancel (默认，推迟到 cancel 点再响应)。

cancel 点：POSIX 定义的 cancel 点，都是可能引发阻塞的系统调用。

```C
int pthread_setcancelstate(int state, int *oldstate); // 设置是否允许取消

int pthread_setcanceltype(int type, int *oldtype); // 设置取消方式

void pthread_testcancel(void); // 什么都不做，就是一个取消点
```

如果在线程取消时已经注册了清理函数，那么就会执行清理函数。

**线程分离**

`int pthread_detach(pthread_t thread);`

不关心它的生死存亡，再也不管它，这就是线程分离的属性。不能再收尸回来（即 pthread_join()）。

线程竞争：多个线程打开同一个文件进行读写操作,见 add.c，会出现覆盖的现象。

池类找质数

将数字放入池中，多个线程从池内拿数字，>0 表示有效，线程判断是否为指数；=0 表示数据被取走；<0 表示已经没有可处理的数据，准备退出。

关于线程的实例：primer0.c 中，这个程序运行时会有问题，结果的数目对不上，而且每次的结果几乎都还不一样，这是因为线程发生了竞争，导致发生这种现象。

程序中的线程处理函数是临界区，即两百零一个线程都执行那一份代码，那么再细分以下，这段代码中其实 i 才是临界资源，因为这些线程在传参时使用地址传参的，所以都在用同一个 i ，这就有可能导致每个线程都还没用完自己的 i 值，然后就被其它线程抢去了，那么该怎么做呢？

我们可以用值传递，那么这样就不是用的同一个 i 了，即用强转类型实现。传参时传入 `(void *) i`, 然后在处理函数中再将 `void *` 强转成 int ，即 `int i = (int) p;`。

第二种则是用动态分配内存，为每一个线程的 i 分配一个地址，都有自己的空间，这里采用了一个结构体，具体程序在 primer0_e.c 中。

#### 3 线程同步

##### 互斥量

程序 add.c 中是没有使用信号量的操作的，出现了线程竞争，即可能会多个线程同时读写文件，导致有些线程写的内容被覆盖。

某个资源在使用时有可能会产生竞争或冲突，则给该资源（临界区）加上锁。

```C
pthread_mutex_t

int pthread_mutex_init(pthread_mutex_t *restrict mutex,const pthread_mutexattr_t *restrict attr);

// pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER; //静态初始化

int pthread_mutex_destroy(pthread_mutex_t *mutex);

int pthread_mutex_lock(pthread_mutex_t *mutex);

int pthread_mutex_trylock(pthread_mutex_t *mutex);

int pthread_mutex_unlock(pthread_mutex_t *mutex);
```

在 add.c 中，由于每个线程都要对文件进行读写，所以读和写的那段代码是临界区，如果这些线程都是只读的，那么可以同时读，这里不是只读的。在程序 add_lock.c 中对临界区锁住，所以不会出现问题。

**例子：** 在程序 abcd.c 中，要求输出按顺序的 abcd 字符，为实现线程同步，这就需要创建时让每一个线程先锁住自己，全部创建完后，首先解锁第一个，写完后第一个负责解锁下一个，以此类推达到同步。

把自己锁住的代码在线程创建时的任务函数中把自己锁住，等全部创建完后，解锁第一个阻塞的线程，即 `pthread_mutex_unlock(mut + 0)`，对于为什么在 for 循环中也要有一个阻塞的代码？

如果你只要在终端输出一次 abcd ，而不是循环输出，其实只需要在 main 的 for 中创建线程前锁住就行，这样锁住的顺序就跟输出顺序一样，解锁的时候也按照输出顺序解锁，在线程处理函数中不用再锁，只需在输出完后解锁下一个锁就行。

如果你要像程序中那样，需要循环按顺序输出 abcd ，那么你就要在线程处理函数的写之前加入锁，这样保证下一轮也是按顺序输出。

**例子：** pth_pool/primer0_pool_busy.c

在临界区内跳转到临界区外尤其要注意，如果没有解锁就跳出去，会发生死锁，所以跳出前要解锁，临界区内跳转没事。如下面一段代码就是临界区的代码。该程序在 pth_pool 文件夹中的 primer0_pool_busy.c 简单模拟线程池的实现。

```C
pthread_mutex_lock(&mut_num);
	while (num == 0) {
		// 如果为 0，解锁让 main 有机会分配
		pthread_mutex_unlock(&mut_num);
		sched_yield();
		pthread_mutex_lock(&mut_num);
	}
	
	if (num == -1) {
		// 不能直接 break 因为还没有解锁，其它线程会被阻塞
		// 在 mut_num 中。
		pthread_mutex_unlock(&mut_num);
		break;
	}
	i = num;
	num = 0;
	pthread_mutex_unlock(&mut_num);
```

还有一处是要注意的，在 main 线程的 num = -1 那里，这是对临界资源的写操作，肯定也要加锁。

```C
pthread_mutex_lock(&mut_num);
num = -1;
pthread_mutex_unlock(&mut_num);
```

但是这样依然不对，因为当最后一个任务分配给 num 后，main 肯定不能就直接赋值给 num 为 -1，万一线程都还没有从 num 中取完那最后一个值，这样就导致最后一个任务被覆盖了。所以也要等判断到 num == 0 时才能确保值被取走了。

```C
pthread_mutex_lock(&mut_num);
while (num != 0) {
	pthread_mutex_unlock(&mut_num);
	sched_yield();
	pthread_mutex_lock(&mut_num);
}
num = -1;
pthread_mutex_unlock(&mut_num);
```

**例子：** 更改 mytbf 中的程序，改为多线程并发版本 mytbf_mt。只要将里面的信号部分转化为线程，用多线程来解决问题。

在用信号来解决的时候，signal 是每秒发一个信号的，所以我们在用多线程解决的时候，用一个线程专门来实现每秒计时。

程序中有 `job[MYTBF_MAX]` 是临界资源，同时只能有一个线程访问。如 **刚开始两个线程同时找 job ，都发现第一个位置为空，然后就都在那里分配内存，这样必然出错。**

还需要注意的一点就是临界区内的代码越短越好，让线程排队的时间短一点，这样就能缩短程序运行的时间，所以尽量优化以下临界区内的代码。如下面在 mytbf_init 中的代码：

```C
// 找空位
pthread_mutex_lock(&mut_job);
pos = get_free_pos();
if (pos < 0) return NULL;

me = malloc(sizeof(*me));
if (me == NULL)
	return NULL;

me->token = 0;  // 一定是从零开始
me->cps = cps;
me->burst = burst;
me->pos = pos;

job[pos] = me;
pthread_mutex_unlock(&mut_job);
```

中间的代码与 job 有关的代码就只有 

```C
pos = get_free_pos();
if (pos < 0) return NULL;
me->pos = pos;
job[pos] = me;
```

我们把其它代码放到加锁的前面。还要注意临界区内，如果失败了要记得解锁和释放刚刚申请的资源。

**其中函数跳转也是一个问题，有可能函数出错，那么就永远回不来了。在 get_free_pos() 函数中也使用到了 job，但是在函数里面是没有必要加锁的，因为在进入函数之前就已经加锁，但是我们要防止的是有人没有加锁就使用这个函数，而这个函数的实现中是没有加锁的，所以我们在函数的命名中要体现出来这个函数的实现是没有加锁的。则改为 get_free_pos_unlocked()，这样来约定俗成函数本身没有加锁，但是调用它前是要加锁的。**

注意这里如果 pos < 0 会跳出去，此时要解锁。

```C
if (pos < 0) {
	pthread_mutex_unlock(&mut_job);
	free(me);
	return NULL;
}
```

在实现 mytbf_fetchtoken() 的时候，以前我们是用 pause() 来等一个信号来处理的，但是现在我们没有用信号了，而且有一个单独的线程代替信号处理函数，这个线程每秒都要进行递增 token，而 fetchtoken 中也用到了 token ，所以这里会有一个冲突，所以 token 也是一个公共资源。**即多个线程不能同时处理一个元素的 token ，但是可以同时处理不同元素的 token ，所以结构体中可以加上一个锁变量 mut。**

所以我们在结构体当中给各自的成员加上一个自己的锁，而且在 mytbf_init 中进行初始化锁，而不是在结构体中初始化，且要在 destroy 函数释放单个令牌桶时同时也要将自己的互斥量销毁。

还有一个要注意的点就是只能调用一次 module_load 函数，所以定义了 inited 标志，明显这里也是要原子操作的，当然这里可以用第三把锁来实现。这里使用到了一个函数 pthread_once，它将你给的函数只调用一次。

```C
int pthread_once(pthread_once_t *once_control,
           void (*init_routine)(void));
```

保证你给的这个函数只调用一次。

目前使用的都是查询法，cpu忙等

##### 条件变量

```C
pthread_cond_t

int pthread_cond_destroy(pthread_cond_t *cond);
int pthread_cond_init(pthread_cond_t *restrict cond, 
					  const pthread_condattr_t *restrict attr);
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;

//广播，叫醒被该条件变量阻塞的线程
int pthread_cond_broadcast(pthread_cond_t *cond);
//叫醒任意一个该条件变量阻塞的线程，随机
int pthread_cond_signal(pthread_cond_t *cond);

//死等
int pthread_cond_wait(pthread_cond_t *restrict cond, 
					  pthread_mutex_t *restrict mutex);
//有时间限制
int pthread_cond_timedwait(pthread_cond_t *restrict cond, 
						   pthread_mutex_t *restrict mutex, 
						   const struct timespec *restrict abstime);
```

**改写之前的例题：**

例如在 mytbf_mt 中的程序：

```C
while(me->token <= 0) {
	pthread_mutex_unlock(&me->mut);
	sched_yield();
	pthread_mutex_lock(&me->mut);
}
```

这一段是忙等的，所以用条件变量来代替应该这样：

```C
while(me->token <= 0) {
	//当 token <= 0 时，me->mut 解锁，让其它线程能进去改变
	//token 值，然后这里等待信号（条件变量）通知（阻塞在条件
	//变量上），等到一个通知后，先抢锁，然后判断 token 是否成立
	pthread_cond_wait(&me->cond, &me->mut);
}
```

之所以不是忙等，应该这样来解释，如果条件不满足，那么解锁 me->mut ，让其它线程可以进去，然后这个线程阻塞在 me->cond 中等待，直到另一边改变了 token 然后发信号通知后，再解锁 me->cond，给 me->mut 加锁，最后判断 token 值。所以说这个线程是阻塞在 me->cond 中，而不是一直运行判断的，则不是忙等。

另一边涉及到改变 token 值的地方要在临界区解锁前要进行发信号，即 pthread_cond_broadcast 。程序详见 mytbf_cond 。

改写 abcd.c

前面我们实现 abcd.c 使用锁链来写的，即让它们在锁链上排序，但是这种方法不太靠谱，所以这里用条件变量来实现。

由于没用锁链了，所以不用四个互斥锁，只需要一个互斥锁就行，用来锁住临界区的代码，即向终端写的那部分代码，只有轮到自己写是才可以进入临界区写。那么怎么来实现呢？

还要定义一个条件变量来通知线程轮到自己了，其中在 main 中还是要创建四个线程各自执行，主要的部分在线程的处理函数中。那么没有了锁链顺序，怎么来让它顺序打印呢？

```C
while(1) {
	pthread_mutex_lock(&mut);
	while(num != n) pthread_cond_wait(&cond, &mut);
	write(1, &c, 1);
	num = next(num);
	pthread_cond_broadcast(&cond);
	pthread_mutex_unlock(&mut);
}
```

那就是在锁住 mut 后，里面再加一个循环 while(num != n) 只要 num (条件变量，每次打印完会递增来达到顺序，然后通知) 不等于线程号 n 就锁在条件变量 cond 中，然后解锁 mut，只有当 num == n 说明此次轮到了自己，才开始抢锁 mut ，抢到后执行打印，然后递增 num 让下一个线程能运行，所以用 num 与 n 是否相等来排序，虽然四个线程异步执行，但只要不是自己的 num 就会被阻塞在 num 中。程序在 abcd_cond.c 中。

##### 信号量

信号量数组，之后进程间通信会涉及到，互斥量是以独占形式使用资源，信号量是资源有限制数的共享，样例使用互斥量+条件变量来实现。

**例子：** 在 primer0.c 中，会创建 201 个线程（显然不合理），之前提到了用交叉算法，分块算法，池类算法解决，那么这里又有一种解决方法，那就是我们可以给你创建这么多线程，但是同一时刻我只允许你最多创建 N 个线程，即给了一个资源上限，这个就可以用信号量来解决。程序详见 mysem。

读写锁

读锁，相当于共享锁，信号量机制，有个数上限控制

写锁，像是互斥锁

容易出现写锁饿死的现象，当有读锁时，加写锁后，后来又来读锁，后来的读锁会继续加锁成功，则读锁一直存在，写锁一直加不上。

在外面再加一层锁，使后面来的排队，这样就不会导致写饿死，先来先解锁。

#### 4 线程属性

##### （1）线程同步的属性

```C
 // 成功返回0 失败返回非0
int pthread_attr_init(pthread_attr_t *attr);

int pthread_attr_destroy(pthread_attr_t *attr);

SEE ALSO:
// 对属性控制的方法，相当于 C++ 类的 public 中的公共成员

// 获取线程分离状态
pthread_attr_setdetachstate() 
pthread_attr_setstack() 
pthread_attr_setstacksize()
......

```

互斥量属性

```C
int pthread_mutexattr_init(pthread_mutexattr_t *attr);
int pthread_mutexattr_destroy(pthread_mutexattr_t *attr);

// 是否跨进程使用
int pthread_mutexattr_getpshared(const pthread_mutexattr_t * restrict attr, 
								 int *restrict pshared); 

int pthread_mutexattr_setpshared(pthread_mutexattr_t *attr, 
								 int pshared);
```

条件变量属性

```C
int pthread_condattr_init(pthread_condattr_t *attr);
int pthread_condattr_destroy(pthread_condattr_t *attr);
```

#### 5 重入概念

##### （1）多线程中的 IO

标准 IO 都已经实现了多线程并发，本身已经实现了 lock 缓冲区，对应有不锁的函数。

例如：

```C
puts(aaaaa);
puts(bbbbb);
puts(ccccc);

// 结果可能为：
aaaaabbbbbccccc
aaaaacccccbbbbb
......

// 但是不可能这样交叉：
abbbaaacccbbacc
......
```

正如上面所说，标准 IO 支持多线程并发，即每个线程在往输出缓冲区写入之前会有一把锁，抢到的线程会把自己要输出的内容一次性全部写进去，此时不会被其它线程打断，等自己写完后，才会解锁给下一个线程，所以不会出现交叉输出的情况。

##### （2）线程与信号

每个线程也有  mask 与 pending 位图，进程只有 pending（全为 0）位图。由内核返回用户态时，计算两次按位与。

以进程为单位时，实际上是没有 mask 位图的，只有 pending 位图。那么进程要是收到信号怎么看屏蔽与否？原因是内核调度的单位已经是线程了，所以用到的是线程的 mask。

那么谁来响应进程范围的信号？**当从内核态回到用户态时，要看调度的时哪个进程中的线程，用这个线程的 mask 先与它隶属于的进程中的 pending 做按位与，看以进程为单位收到哪个信号，然后再让这个线程的 mask 与这个线程的 pending 做按位与**。所以应该是做了两次按位与。

```C
pthread_sigmusk()
sigwait();
// 给线程发信号
pthread_kill() 
```

##### （3）线程与 fork

一个进程中有多个线程时，fork 此进程，新进程有几个线程？

答：不同的标准实现不同。

之前的很多程序都可以用多线程实现，在 anytimer 中有个语句需要用多线程或多进程实现
