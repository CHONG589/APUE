Inter-Process Communication 进程间通信

[进程间的通信方式全面解析](https://blog.csdn.net/u014571143/article/details/129580485)

### 1. 管道（使用内核缓冲区传输数据）

由内核提供， 半双工（一端读、一端写），自同步机制（处理速度由慢的那方决定）。

#### 匿名管道

不能用在没有亲缘关系的进程中。（使用 ls 看不到文件）

```C
//下标为 0 的为读端，下标为 1 的为写端，如果使用父进程写，子进程读时，最好是父进程的读端关掉
//子进程的写端关掉，这样才不会影响。
int pipe(int pipefd[2]); 
```

程序在 6_Ipc/pipe/ 中，里面有两个程序，其中，player.c 中的是没写完的，从网上下载歌曲的过程没写，用了伪码表示。

#### 命名管道

（文件类型为 p 的文件），如：`prw-rw-r--` 。

```C
// mode：操作权限。（mode & ~umask）
int mkfifo(const char *pathname, mode_t mode);
```

### 2. XSI -> SysV

进程通信的三种机制：使用 ipcs 查看

- 消息队列：Message Queues

- 信号量数组：Semaphore Arrays

- 共享内存段：Shared Memory

这三个都是通过创建实例、不同进程操作实例来完成。如果是有亲缘关系还好说，只要产生 fork() 后，对方就能拿到，那么没有亲缘关系的怎么拿到呢？这就用到了 key 这个字段，key 值得产生，是为了确定通信的双方拿到同一个通信机制来进行实现。

实例的创建先使用 ftok() 生成 key，然后再用 key  生成 msgid、semid 或 shardid。

```C
key_t ftok(const char *pathname, int proj_id);
```

#### Message Queues

实现 Message Queues ，会用到下面几个函数。这几个函数中在 Semaphor Arrays、Shared Memory 中其实都是类似的。

```
xxxget  ->  创建
xxxopt  ->  操作
xxxctl  ->  控制、销毁

xxx 分别为：msg、sem、shm
```

在 Message Queues 中则为：

```C
int msgget(key_t key, int msgflg);

// op
// 发送
int msgsnd(int msqid, const void *msgp, size_t msgsz, int msgflg);
// 接收
ssize_t msgrcv(int msqid, void *msgp, size_t msgsz, long msgtyp,
		        int msgflg);

int msgctl(int msqid, int cmd, struct msqid_ds *buf);
```

通信双方拿着同一个 key 值就可以进行通信实现通信。

在 /6_Ipc/xsi/msg/basic 中是消息队列的实现，里面规定了 协议 proto.h，和收发双方的程序。但是创建的消息队列即使最后用 Ctrl + C 终止后，用 ipcs 还是可以看到这个消息队列存在，因为这是异常终止，并没有执行 msgctl()，所以还会存在，可以用信号解决，当接收到某个信号后，执行对应的处理函数，还可以使用 shell 命令在终端中执行 ipcrm 删除消息队列、信号量数组、共享内存，例如用 ipcrm -q 。

这个消息队列的实现是没有互动的，即发送消息的只能不断的发，收消息的只能收，而不能做到既收也可以发的互动操作。

下面写一个类似 ftp 的实现，即有互动的操作，既可以发，也可以接收。其中当文件过大时，可以一段一段发，直到发完，结束用 EOF 标记。这里只规定了协议的格式和包的实现方式，具体例子没有写。在 6_Ipc/xsi/msg/myftp_msg 中，其中比较推荐 proto1.h 中的方式，采用 union。

#### Semaphore Arrays

信号量数组就是操作系统中的信号量的 P() 和 V() 操作，对资源量的操作。

使用它同样涉及到三个函数

```C
semget()
semop();
semctl();
```

具体例子在 /6_Ipc/xsi/sem/add.c 中。

#### Shared Memory

比 mmap 共享内存复杂一点，mmap 的具体看  [[09_高级IO专题#4. 存储映射 IO]]

```C
shmget();
shmop();
shmctl();
```

这里使用 mmap 中的例子来用 shm 来重构一下。

### 3. 网络套接字

推荐书籍：《unix 网络编程》、《tcp/ip 网络协议》的第一卷读下来就可以。

#### 跨主机的传输要注意的问题

1. 字节序问题：

大端：低地址处放高字节

小端：低地址处放低字节

主机字节序：host

网络字节序：network

因为每台机器采用的字节序可能会不一样，这样进行通信时，就会造成错误，所以为了统一标准，这里规定了主机字节序、网络字节序（在网络上传输时的序列），发送时要从主机序列转换成网络序列，接受时则要先把网络序列转换成本机的主机序列。`htons, htonl, ntohs, ntohl`。

2. 对齐：

```C
struct {
	int i;
	float f;
	char ch;
};
```

结构体在内存中存储时，机器为了加快当前的取指周期，会进行内存对齐。但是凡是参与网络通讯的这种结构体，要禁止对齐。因为每台机器采取的对齐方式不一样，且存储的位置不一样，最终计算出来的大小可能也会不一样，所以在网络通讯的结构体中不对齐。（通过指定宏让编译器不对齐）

3. 类型长度问题：

各种类型的具体长度不确定，只规定了大概范围，不同位数的机器不一样。

解决方法：int32_t, uint32_t, int64_t, 采用这些通过指定占多少位数的类型。

#### socket 详解

socket 是什么？（中间层）

不同的协议都会支持多种不同的传输方式，如 IPV4 支持流式传输，报式传输等，如果这样的话，程序员都要针对不同的协议和对应的传输方式去编写对应的程序，这样就很麻烦，一换协议或者传输方式，就得再编写程序。

socket 机制：将协议和实现方式（流式、报式）封装起来，成为一个fd，这样就相当于一个 IO 流，FILE * 的 fd，可以用标准 IO 和系统 IO 来操作，体现了一切皆文件。

##### socket 函数

```Cpp
// Linux下面一切都是文件 
// 无论是管道，文件还是网络的数据，都是将其当成文件 
// 会通过句柄来操作这些文件的读写（将接口统一） 
// (LINUX 环境编程) 
#include <sys/types.h> 
#include <sys/socket.h>

int socket(int domain, int type, int protocol);

- domain:
    - AF_INET:  这是大多数用来产生socket的协议，使用TCP或者UDP来传输，用IPv4的地址
    - AF_INET6: 与上面的类似，不过是采用IPv6的地址
    // 若是本机的 a 进程和 b 进程之间进行通信就无需经过网络了（不会经过网络协议栈）      
    // 那就会在 a 和 b 之间，在内部就建立一个通道在内核的高速缓存这一层（通过内核进行通信，效率会更高） 
    // 所以本地间的通信也可以创建一个 AF_UNIX         
    - AF_UNIX:  本地协议，使用在 Unix 和 Linux 系统上，一般都是当客户端和服务器在同一台机器上的时候使用
	// TCP 协议按照顺序保证数据传输的可靠性，同时保证数据的完整性

- type:
    - SOCK_STREAM:(面向流)  这个协议是按照顺序的、可靠的、数据完整的基于字节流的连接。使用经常，即socket使用TCP进行传输
    // 常常是 stream（用于 TCP 连接）或 dgram（用于UDP服务）
    // 数据报 dgram        
    - SOCK_DGRAM:(面向数据报)   (数据用户报文)这个协议是无连接的(即UDP协议)，固定长度的传输调用。该协议是不可靠的，使用UDP来进行它的连接。流媒体领域用的多（抖音，直播, 迅雷下载等等），因为它不保证这个数据的可靠性，速度快;但是udp协议是不可靠的，面向无连接, 但是比如迅雷下载在应用层有应用层的协议来保障数据传输的可靠性，应用层协议在udp之上。      
    - SOCK_SEQPACKET: 该协议是双线路的、可靠的连接，发送固定长度的数据包进行传输。必须把这个包完整的接受才能进行读取。  
    - SOCK_RAW: socket类型提供单一的网络访问，这个socket类型使用ICMP公共协议。(ping、traceroute使用该协议)
    - SOCK_RDM: 这个类型是很少使用的，在大部分的操作系统上没有实现，它是提供给数据链路层使用，不保证数据包的顺序。      
- protocol: 
    - 传0表示使用默认协议。
- 返回值:
    - 成功: 返回指向新创建的socket的文件描述符; 失败: 返回-1, 设置errno
```

**socket() 打开一个网络通讯端口，如果成功的话，就像 open() 一样返回一个文件描述符，应用程序可以读写文件一样用 read/write 在网络上收发数据，如果 socket() 调用出错则返回 -1。**

##### bind 函数

_通常服务器在启动的时候都会绑定一个众所周知的地址（如 ip 地址 + 端口号），用于提供服务，客户就可以通过它来接连服务器；而客户端就可以不用指定，有系统自动分配一个端口号和自身的 ip 地址组合。这就是为什么通常服务器端在 listen 之前会调用 bind()，而客户端就可以不调用，而是在 connect() 时由系统随机生成一个。_

服务器会有成千上万的客户端连接，随意变动 IP 地址都得重新通知客户端，这样就会很麻烦；

一个服务器上还有很多的网卡，多个网络接口上都对应着一个 IP 地址，不同的网络接口提供不同的服务，绑定的 IP 地址不一样；

服务器程序所监听的 **网络地址** 和 **端口号** 通常是 **固定不变** 的，客户端程序得知服务器程序的地址和端口号后就可以向服务器发起连接，因此服务器需要调用 bind 绑定一个固定的网络地址和端口号。

**注意：这里是指 IP 地址会随意变动，但是一般服务器的网络地址和端口是不变的，所以通过绑定这个来确定位置。**

```Cpp
#include <sys/types.h>
#include <sys/socket.h>

// addrlen 就是地址的长度
int bind(int sockfd, const struct sockaddr *addr, socklen_t addrlen); 
- sockfd:
    socket 文件描述符 （句柄）
- addr: 
    构造出IP地址+端口号，这个地址要强制转换成 sockaddr
- addrlen:
    sizeof(addr)长度
- 返回值:
    成功返回0， 失败返回-1， 设置errno
```

**bind() 的作用是将参数 sockfd 和 addr 绑定在一起，使 sockfd 这个用于网络通讯的文件描述符监听 addr（地址）所描述的地址和端口号。**

**前面讲过，struct sockaddr 是一个通用指针类型，addr 参数实际上可以接受多种协议的 sockaddr 结构体，而他们的长度各不相同，所以需要第三个参数 addrlen 指定结构体的长度**

addr 具体使用什么结构体要看你用到了哪个地址族，如用 AF_INET，它的 addr 结构体如下：

```Cpp
struct sockaddr_in {
sa_family_t sin_family; /* 地址族: AF_INET */
u_int16_t sin_port; /* 按网络字节次序的端口 */
struct in_addr sin_addr; /* internet地址 */
};

/* Internet地址. */
struct in_addr {
u_int32_t s_addr; /* 按网络字节次序的地址 */
};
```

在网络通信中，为了保证数据在不同主机之间的正确传输，需要将数据转换为 **网络字节顺序（大端序）** ，然后再进行传输。接收方在接收到数据后，需要将数据转换为 **主机字节顺序（根据本地字节顺序决定是大端序还是小端序）** 进行处理。

因此，在发送和接收网络数据时，必须将数据从主机字节序转换为网络字节序，并在接收数据时将其转换回主机字节序。htons 和 htonl 是用于这种转换的函数。

上面 struct sockaddr_in 结构体中，sin_family 是字符型的，猜测是单字节的，所以可以不用转换字节序，而 sin_port 和 sinaddr 分别是 16 位和 32 位，所以要转换字节序，且它那里也明确解释要转换成网络字节序。

而 inet_pton 函数在将点分 IP 地址转换成二进制字节流的时候，这个二进制字节流就已经是网络字节序的二进制格式。inet_pton 函数同样也一样，将网络字节序的二进制格式转换成可读的点分 IP 地址形式。

##### 接收消息函数

```Cpp
ssize_t recvfrom(int sockfd, void *buf, size_t len, int flags, 
				 struct sockaddr *src_addr, socklen_t *addrlen);

buf：存放接收消息的缓冲区。
len：buf 的大小
arc_addr：接收消息后存放对方发消息的地址区域，如果为 NULL，不存储对端地址。
addrlen：arc_addr 的长度。
```

##### 发送消息函数

```Cpp
ssize_t sendto(int sockfd, const void *buf, size_t len, int flags,
	            const struct sockaddr *dest_addr, socklen_t addrlen);

buf：要发送消息存储的地址。
len：buf 的大小。
dest_addr：存储的地址，即要发送的目标地址。
addrlen：dest_addr 的大小。
```

#### 报式套接字

双方的第二部取地址指的是绑定本地的一个地址，也就是端口。被动端可以想象成是 serve 端，然后主动端想象成是 client 端，对于服务器端的话肯定得要先分配一个知名的端口，让大家都知道它，然后知道了后，大家访问它才知道在哪，而客户端的话如果你自己没分配的话，主机会自动分配一个没人用的端口给它。当然 serve 端也要先运行起来。

**被动端（先运行），先收包的一方**

1. 取得 socket
2.  给 socket 取得地址
3.  收/发消息

**主动端**

1. 取得 socket
2. 给 socket 取得地址（可以省略，因此系统会给进程分配端口）
3. 收/发消息
4. 关闭 socket

实现 UDP 的报式套接字的程序在 6_Ipc/socket/dgram/basic 中。

上面那个程序中协议里的实现还是有点死板，就是包里面的名字大小固定死了，可能会想到用指针，但是这不对，因为你在本机分配了内存，你在包里存的是地址，你把地址传过去是怎么回事，你的数据都还在本机，目的主机的这个地址都没有你的数据，你必须要把数据传过去才对。

这里采用了以前数据结构实现封装的方法，用一个标志占位，初始化的时候再为它分配内存，把分配的内存存进结构体里，而不是只给一个地址。

所以在 sento 发送时用来存放发送数据的 buf 不能用 sizeof(buf) 来计算大小了，因为用这个计算的大小中，name 字段只占一个字节大小，不是实际的，要加上实际名字的大小。

然后在 rcver 中，由于不知道发送方会发送多少数据，因为结构体的大小不确定，所以就按最大 UDP 报文大小来接收，即为 buf 分配 sizeof(struct msg_st) + NAMEMAX - 1 的大小。

具体程序在 6_Ipc/socket/dgram/var 中。

##### 多点通讯（流式不能实现）

广播（全网广播，子网广播）、多播/组播

**广播实例**

要实现广播，我们在 /dgram/basic/ 的例子上加以改，在 /6_Ipc/socket/dgram/bcast 中。

在 snder.c 中我们要把发送的 IP 地址改为广播地址，即 255.255.255.255，然后还要把广播的开关打开，要用到这个函数。

```Cpp
int getsockopt(int sockfd, int level, int optname,
			  void *optval, socklen_t *optlen);
int setsockopt(int sockfd, int level, int optname,
			  const void *optval, socklen_t optlen);
```

这里用第二个函数即可，将广播标志位打开。在 rcver.c 中也需要用第二个参数将广播标志打开，这时运行 snder 时就不用加 IP 参数了。

**多播实例**

在 6_Ipc/socket/dgram/mcast 中。

#### 流式套接字

**C 端（主动端）**

1. 获取 socket
2. 给 socket 取得地址（可省略，省略后会自动分配一个端口）
3. 发送连接
4. 收/发消息
5. 关闭

**S 端（被动端）**

1. 获取 socket
2. 给 socket 取得地址
3. 将 socket 置为监听模式
4. 接收连接
5. 收/发消息
6. 关闭

##### 监听函数

```Cpp
s 为需要进入监听状态的套接字，backlog 为请求队列的最大长度。
int listen(int s, int backlog);

第二个参数 backlog 是队列的大小，等于未完成连接队列(正在等待完成相应的TCP三路握手过程) + 已完成连接队列(已经完成相应的TCP三路握手过程)

backlog 也可以这么理解，一开始设计是为半连接池，但现在相当于全连接池，即 server 所能承受全连接数量的大小。
```

##### 接收连接的函数

当套接字处于监听状态时，可以通过 accept() 函数来接收客户端请求。它从未完成连接队列中取出第一个连接请求,创建一个和参数 s 属性相同的连接套接字，并为这个套接字分配一个文件描述符, 然后以这个描述符返回.新创建的描述符不再处于倾听状态。原套接字 s 不受此调用的影响。如果连接队列为空，则阻塞。

```Cpp
//s：为服务器端套接字
//addr：sockaddr_in 结构体变量，填充对方的地址，和对方的信息。
//addrlen：参数 addr 的长度，可由 sizeof() 求得。
int accept(int s, struct sockaddr *addr, socklen_t *addrlen);

//成功返回已连接套接字，并变为主动套接字。
```

**注意: accept() 返回一个新的套接字来和客户端通信，addr 保存了客户端的 IP 地址和端口号，而 s 是服务器端的套接字，注意区分。后面和客户端通信时，要使用这个新生成的套接字，而不是原来服务器端的套接字。**

最后需要说明的是：listen() 只是让套接字进入监听状态，并没有真正接收客户端请求，listen() 后面的代码会继续执行，直到遇到 accept()。**accept() 会阻塞程序执行（后面代码不能被执行），直到有新的请求到来**。

##### connect 函数

```Cpp
int connect(int sockfd, const struct sockaddr *addr,
            socklen_t addrlen);
```

`connect`函数用于客户端建立 tcp 连接，发起三次握手过程。其中`sockfd`标识了主动套接字，`addr`是该套接字要连接的主机地址和端口号，`addrlen` 为 `addr` 缓冲区的长度。

流式实现的方法在 6_Ipc/socket/stream/basic 中。

但是上面这个程序会有一个问题，就是在 server.c 中的 server_job 函数进行发送的代码中，如果里面加上一个 sleep(10)，那么这样每次都要等一个请求运行十秒然后下一个请求才能运行，所以这样很浪费资源，现在要求实现一个并发版的，每次运行 server_job 函数就用一个线程去执行。父进程负责 accept，成功了就把工作交给子进程。6_Ipc/socket/stream/par。

[wireshark 使用详解](https://blog.csdn.net/qq_42257666/article/details/129089264)

[配置 apache 详解](https://veitchkyrie.github.io/2019/10/24/Ubuntu%E9%85%8D%E7%BD%AEApache-CGI%E7%A8%8B%E5%BA%8F/)，将 cgi-bin 目录配置到 /var/www/ 目录下。

用浏览器来找到本地存的一个图片，启动抓包器把我从网页上访问这张图片的过程截获下来，然后将这个抓下来的内容放在程序中进行回滚，即在程序中复现出来。

将要访问的图片放到 var/www/html/ 目录下，然后通过浏览器访问 `127.0.0.1/文件名` 就可以看到要访问的内容。 

在 6_Ipc/socket/stream/webdl.c 文件中的程序是使用 http 协议将浏览器访问本地 127.0.0.1 目录下的文件时的过程用抓包器抓到的内容，然后用 webdl.c 文件进行内容回滚的过程。

实例：

- /6_Ipc/socket/stream/pool_static 中的静态静态进程池的实现。
