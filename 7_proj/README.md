### Server端
#### 媒体库 medialib 模块（存放所有待播放的数据）， 可以使用数据库（如 mysql ）或直接使用文件系统

当使用文件系统时，可以用专门一个目录来存放所有的频道，每个频道都是一个子目录。

在数据通过 socket 往外发时，需要考虑 **流量控制**，主要是因为：

1. 假设客户端全部收到，在播放时有可能会发生破裂，播放速度有快有慢。
2. 通过 socket 发送有包的大小限制，太大了有可能会失败。

#### thr_channel 模块（多线程推节目数据）

使用多线程 + 流控，将频道里的数据发送出去。所以可以通过一个线程或者多个线程来控制每秒发多少数据来实现。

#### thr_list 模块（多线程推节目单）

由于 server 端中的频道对于 client 来说是隐藏的，所以还要一个节目单模块来发给 client 端。当然不能只在开始时发送一个就不发送了，因为有些 client 端不一定是从开始加入进来的，可能是中间某个时间加入进来的，它们也要节目单。可以一秒发一次。

节目单也是通过频道推出，设为 0 号频道？

主要逻辑，客户端收到节目单，选择一个频道进行收听，其他频道不受影响（可以理解为并发播放），继续 C 端的选择不必告诉 S 端

因为客户端没有暂停的功能，如果有暂停，则成为点播的系统。

（看不是目的，只有自己写才能了解真正内容）

### 项目目录结构

- INSTALL：部署指导
- LISENCE：使用许可，遵循的协议
- README：介绍
- Makefile：
- doc（目录）：有关项目的文档
	- admin：管理员看的
	- devel：同行看的
	- user：用户看的
- src（目录）：源码
	- include（目录）：两者约定的协议
		- proto.h
		- site_type.h：协议中数据类型定义的文件
	- client（目录）
		- client.c
		- client.h
		- Makefile
	- server（目录）
		- server.c
		- thr_list.c
		- thr_list.h
		- thr_channel.c
		- thr_channel.h
		- medialib.c
		- medialib.h
		- server_conf.h
		- mytbf.c
		- mytbf.h
		- Makefile

```Cpp
.
└── netradio
    ├── doc
    │   ├── admin
    │   ├── devel
    │   └── user
    ├── INSTALL
    ├── LISENCE
    ├── Makefile
    ├── README
    └── src
        ├── client
        │   ├── client.c
        │   ├── client.h
        │   └── Makefile
        ├── include
        │   ├── proto.h
        │   └── site_type.h
        └── server
            ├── Makefile
            ├── medialib.c
            ├── medialib.h
            ├── server.c
            ├── server_conf.h
            ├── thr_channel.c
            ├── thr_channel.h
            ├── thr_list.c
            └── thr_list.h
```
