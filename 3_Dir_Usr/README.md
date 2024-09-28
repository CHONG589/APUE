### 系统数据文件和信息

在实现 ls 命令的时候获取 uid 或者 gid 等星系的时候，不能直接到 /etc/passwd 或者到 /etc/groud 文件中取获取这些信息，并不是每个系统都有该文件，但是有相应的标准库函数可以获取这些信息，即使每个系统不一样，但都得按照标准实现对应的库函数操作。

#### 1.  /etc/passwd

获取用户信息函数，使用见 getuname.c 文件，有 getpwuid(); 等函数。

#### 2. /etc/group


#### 3. /etc/shadow

该文件是 Linux 系统中存储用户加密密码的文件，每行对应一个用户的加密信息。该文件只有 root 用户可以访问，但是没有执行权限，普通用户则任何权限都没有。

getspnam();  只有 root 账号才有权限。细节用 man 查看。

#### 4 时间戳

```C
time_t time(time_t *tloc);

/*
time()  returns  the time as the number of seconds since the Epoch, 1970-01-01
00:00:00 +0000 (UTC).
If tloc is non-NULL, the return value is also stored in the memory pointed  to
by tloc.
*/
// 意思就是这个函数从内核取时间戳，取的时间返回到返回类型中，如果参数不为空，它也会把取
// 到的时间给参数。

struct tm *localtime(const time_t *timep);

struct tm {
   int tm_sec;    /* Seconds (0-60) */
   int tm_min;    /* Minutes (0-59) */
   int tm_hour;   /* Hours (0-23) */
   int tm_mday;   /* Day of the month (1-31) */
   int tm_mon;    /* Month (0-11) */
   int tm_year;   /* Year - 1900 */
   int tm_wday;   /* Day of the week (0-6, Sunday = 0) */
   int tm_yday;   /* Day in the year (0-365, 1 Jan = 0) */
   int tm_isdst;  /* Daylight saving time */
};
```

#### 进程环境

##### 1. main函数
##### 2. 进程的终止

**正常终止：** 

(1) 从 main 函数返回，如 return 0;

(2) 调用 exit

标准库函数。

```C
void exit(int status);

// status 为 32 为，看似它好像可以表示 2^31 种状态，其实不然，它要
// 进行 status & 0377(八进制) ，只能保留低八位，有符号的 char 型。

void atexit(void (*function)(void))    // 钩子函数
```

钩子函数：一个进程在 **正常终止之前**，会调用钩子函数去释放所有该释放的内容。类似于 C++ 的析构函数。atexit 函数是一个特殊的函数，它是在 **正常程序退出时** 调用的函数，我们把他叫为登记函数。

一个进程可以登记若 32 个函数，这些函数由 exit 自动调用，这些函数被称为终止处理函数，atexit 函数可以登记这些函数。exit 调用终止处理函数的顺序和 atexit 登记的顺序相反，如果一个函数被多次登记，也会被多次调用。

作用：当程序因为打开文件失败时，判错时发现没打开，所以提前 exit(1) ，这时可以用钩子函数，不用在 exit(1) 前一个一个加 close（若打开的文件过多，那么就要写很多）。在每打开一个文件后就用 atexit 注册钩子函数，在传参的函数里面调用 close，这样就会自动关闭。

[参考例子](https://blog.csdn.net/xuechanba/article/details/119972313)

(3) 调用 `_exit` 或者 `_Exit`

系统调用函数。`_exit` 和 `_Exit` 直接调用内核，不执行钩子函数和 io 清理函数。如果异常时，不想扩大影响（exit 会刷新缓冲等），则不使用 exit ，使用 `_exit` 。

(4) 最后一个线程从其启动例程返回

(5) 最后一个线程调用 pthread_exit

**异常终止：** 调用 abort、接到一个信号并终止、最后一个线程对其取消请求做出响应。

##### 3. 命令行参数的分析

getopt

##### 4. 环境变量

实质就是 key = value，使用 export 查看

```C
getenv();

char *getenv(const char *name);

setenv(); 新增或修改环境变量

int setenv(const char *name, const char *value, int overwrite);

//overwrite决定是否覆盖

//修改的环境变量，是在堆上重新申请一块区域存储，而不是在原来的存储上修改

putenv();

int putenv(char *string); // 行参不是用 const 修饰，一般不用
```

##### 5. C 程序的存储空间布局

##### 6. 库

动态库

静态库

手工装载库

##### 7. 函数跳转

goto;

setjmp();

longjmp();

##### 8. 资源的获取及控制

ulimit -a

getrlimit();

setrlimit();
