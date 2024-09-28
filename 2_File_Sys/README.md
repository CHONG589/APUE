可用的文件IO函数--打开、读、写等。

linux大多数文件IO只需用到5个函数：open read write lseek close，然后说明**不同缓冲区长度对read和write函数的影响**。

本章所说的函数被称为不带缓冲的IO，与标准IO相对应，它是指每次read和write时立即调用内核中的系统调用。

涉及到多个进程间共享资源，原子操作的概念就变得非常重要。将通过文件IO和open函数的参数来讨论此概念。

### 文件描述符

所有打开的文件都通过文件描述符引用。它是一个非负整数，当打开或创建时一个文件时，内核向进程返回一个文件描述符。

ssize_t read(int fd, void *buf, size_t count);  // 从fd读取count字节大小到buf中

ssize_t write(int fd, const void *buf, size_t count); // 从buf中取count字节大小写到fd中

### 文件共享

(1) 每个进程在进程表中有一个记录项，记录项中包含一张打开文件描述符表，每个文件描述符占用一项

(2) 内核为所有打开文件维持一张文件表

(3) 每个打开文件(或设备)都有一个v节点结构。

打开文件描述符表可存放在用户空间，而非进程表中。

两个独立进程各自打开了同一个文件，则有两个文件描述符表，却只有一个v节点表。

一个进程删除或重命名文件时，另一个文件能立即感知到吗？

ps:

文件io函数针对文件描述符，而标准io是围绕流进行的。

经常使用的都是标准IO，其有缓冲区，有三种缓冲模式：全缓冲、行缓冲和无缓冲。

标准IO的实现

### 原子操作

比如一个进程要做copy文件操作，在它copy时，如果另一个进行也在操作此文件。

### 目录和文件（会使用到递归）

#### 1. 获取文件属性

##### ls 命令的实现描述

`cmd(命令) --长格式 -短格式 非选项的传参`

- 长格式： `ls --all` 
- 短格式： `ls -a`

如果只用短格式，有 26 个小写字母和 26 个大写字母， 还可以使用 0-9 的数字， 则选项共有26 * 2 + 10 个选项

例子：创建一个名称为 -a 的文件

直接使用 touch -a 是不成功的，会将 -a 视为选项而非文件名。可使用 touch ./-a (./-a 当成路径) 或 touch -- -a。 

ls -l 和 ls -n  的区别，-l 显示用户名和组名，-n 显示用户号和组号

可通过 /etc/passwd 和 /etc/group 查看名称和编号的对应关系

##### stat、fstat、lstat 查看文件属性

```C
// 通过路径获取属性
int stat(const char *pathname, struct stat *statbuf);

// 通过文件描述符 fd 取属性
int fstat(int fd, struct stat *statbuf);  

// 和 stat 的链接属性不同(面对符号链接文件时，获取的是链接文件的属性)
int lstat(const char *pathname, struct stat *statbuf);  
```

```C
struct stat {
	//包含该文件的设备的设备号
	dev_t     st_dev;     /* ID of device containing file */  
	
	// inode号
	ino_t     st_ino;     /* inode number */   

	// 权限信息
	mode_t    st_mode;    /* File type and mode */ 

	nlink_t   st_nlink;   /* number of hard links */          
	uid_t     st_uid;     /* user ID of owner */             
	gid_t     st_gid;     /* group ID of owner */ 

	// 如果拿到的是设备，则是设备号
	dev_t     st_rdev;    /* device ID (if special file) */ 
	  
	off_t     st_size;    /* total size, in bytes */      

	// 一个block的大小，默认为512字节
	blksize_t st_blksize; /* Block size for filesystem I/O */ 

	// 占用多少个block块
	blkcnt_t  st_blocks;  /* number of 512B blocks allocated */ 

	// 最后一次读的时间
	time_t    st_atime;   /* time of last access */  

	// 最后一次写的时间
	time_t    st_mtime;   /* time of last modification */ 

	// 最后一次亚数据修改的时间
	time_t    st_ctime;   /* time of last status change */      
};
```

之前有两种方式来获取文件大小（哪两种？），现在通过 stat 来获取，见 flen_with_stat.c 文件

ps：安装 tags 工具，则可使用【vim -t 系统重定义的数据类型】命令来查看 typedine 定义的数据类型， 如 vim -t off_t

注意：st_size 只是文件的属性，而 st_blksize 和 st_blocks 才真正决定占用磁盘空间的大小。

【空洞文件】有可能文件特别大，但占用磁盘空间特别小， 见 test_bigfile.c

st_mode 包含了 ls -l 查看的文件类型和文件权限，是一个16位的整形数。

##### 文件类型：dcb-lsp

- d：directory 目录
- c：character 字符设备文件
- b:：block 块设备文件
- -：regular 常规文件
- l：link 符号链接文件，硬链接文件是目录项，不会有任何表示。
- s：socket文件
- p：pipe 管道文件，这里特指命名管道文件，匿名管道在磁盘上看不到

可以用宏判断，也可以按位与操作

#### 2. 文件访问权限

st_mode 是一个 16 位的位图，用于表示文件类型、文件访问权限及特殊权限位（u+s, t+s, 粘住位）

#### 3. umask

创建文件： 0666 & ~umask

umask 既是一个命令，也是一个函数，防止产生权限过松的文件

```C
#include <sys/types.h>
#include <sys/stat.h>

// 用来设置 umask 值
mode_t umask(mode_t mask);
```

#### 4. 文件权限的更改\管理

chmod : 作为命令使用 chmod a + x 或 g + x 或 u + x 或 o + x

所有者：owner，group、other

权限：r (读)、w (写)、x (执行)

- a + x 表示所有人有执行权限
- g + x 表示组成员有执行权限

`chmod a + x big.c` 所有人都有 big.c 的执行权限

`chmod 666 big.c` 将 big.c 的权限改为 666

作为函数：

```C
// 操作文件
int chmod(const char *path, mode_t mode);

// 操作文件描述符
int fchmod(int fd, mode_t mode);
```

#### 5. 粘住位

t 位，保存二进制可执行文件的执行痕迹 (现在不常用)，现在常用给某个目录设置 t 位，如 /tmp 的位图 ：drwxrwxrwt

#### 6. 文件系统：FAT, UFS系统

文件系统：文件或数据的存储和管理

UFS 系统是早期的 linux 文件系统，开源

FAT 系统是同时期的微软文件系统，不开源。

##### FAT16/32 系统

静态单链表，也就是用一个数组存储一个单链表，文件的索引指向第一个节点的下标，第一个节点的 next 值指向第二个节点的下标，以此类推，从而实现链表的特性，存储方式虽然是用数组，但是文件逻辑相邻的区域，实际存储位置不一定相邻。 

缺陷：一个走向、承载的文件大小有限制 。

用途：U 盘，SD 卡。轻量级，占用空间小

##### UFS 系统

就是 408 操作系统里文件那章讲的文件存储方式，有直接索引、一级索引、二级、三级。

一个文件几乎所有的信息都存在 inode 节点中，**除了文件名 (存在目录文件的目录项中)**

12个普通指针，1 个一级块指针，1 个二级块指针，1 个三级块指针

缺陷：

- 小文件不占用过多的磁盘空间，却耗尽了 inode
-  inode 没有用多少，磁盘空间却用尽了

用 inode 位图表示 inode 是否使用，用块位图表示数据块是否使用

不用比较和判断的情况，取出两个32位无符号的大小情况

两个数交换（借助tmp数），或者两数异或

统计一个32位无符号数的二进制表示数中0和1的个数

目录文件：

放在某个目录下的文件，由目录项（inode + 文件名）组成

#### 7. 硬链接， 符号链接

##### 硬链接：

`ln source des`

创建一个硬链接，就是在目录文件中创建一个目录项，其 inode 和原来相同，删除一个后另外一个仍能正常使用，硬链接文件和原文件的属性均相同

##### 符号链接： 

`ln -s source des`

类似于快捷方式，拥有自己的 inode 号，不占用磁盘空间，其 size 为指向文件的文件名长度，删除原文件，符号链接文件不可用。

```C
int link(const char *oldpath, const char *newpath); // 由此封装的 ln

int unlink(const char *pathname); // 可用来创建临时文件 open 后立即执行 unlink

int remove(const char *pathname); // 由此封装的 rm 命令

int rename(const char *oldpath, const char *newpath); // 封装的 mv 命令
```

硬链接与目录项是同义词，且建立硬链接有限制，不能给分区建立(防止 inode 重复)，**不能给目录建立**。符号链接可以跨分区，可以给目录建立链接。

#### 8. 更改时间 utime

#### 9. 目录的创建和销毁

```C
int mkdir(const char *pathname, mode_t mode);
int rmdir(const char *pathname); // 删除空目录
```

#### 10. 更改当前工作路径

```C
int chdir(const char *path); // 封装出 cd 命令

int fchdir(int fd);
```

进程相关的会用到，长时间运行的进程一般放在根目录，因为根目录一般是一直存在的，不想 U 盘解除挂载后就不存在了。

#### 11. 分析目录/读取目录内容

##### 1. 用 glob() 函数解析目录

解析模式/通配符，使用见 glob.c

```C
int glob(const char *pattern, int flags,
        int (*errfunc) (const char *epath, int eerrno),
        glob_t *pglob);
void globfree(glob_t *pglob);

//pattern: 带通配符的模式

typedef struct {
   size_t   gl_pathc;    /* Count of paths matched so far  */
   char   **gl_pathv;    /* List of matched pathnames.  */
   size_t   gl_offs;     /* Slots to reserve in gl_pathv.  */
} glob_t;
```

glob() 特别像 argc 和 argv，一个表示数量，一个表示参数列表。 

写程序时肯定要定义一个 glob_t 结构的变量来接收匹配的结果，而定义这个结构体时会为它分配一个二级指针，是动态分配的内存，所以定义了 glob_t 变量，肯定要自己释放，这里有释放内存的函数 globfree();

##### 2. 用目录流解析目录

```C
opendir();
closedir();
readdir();
rewinddir();
seekdir();
telldir();
```

opendir(): 使用见 opendir.c

```C
DIR *opendir(const char *name);
DIR *fdopendir(int fd);         //通过 fd (即 open 函数打开后得到的文件描述符)打开 dir
```

The  opendir()  function  opens a directory stream corresponding to the directory name, and returns a pointer to the directory stream.  The stream is positioned at the first entry in

the directory.

即将目录名字(要用路径形式传入)传入参数中，该函数会打开目录，并且返回该目录的目录流。返回的目录流指向该目录中第一个条目处，即指向该目录下的第一个目录项。该目录流包含了该目录下的所有文件或目录。

这两个函数成功都返回一个目录流的指针，失败返回 NULL。返回的指针指向的内容存在堆上。

closedir()：关闭目录流。

readdir()：The  readdir() function returns a pointer to a dirent structure representing the next directory entry in the directory stream pointed to by dirp.  It returns NULL on reaching the end

of the directory stream or if an error occurred.

readdir() 函数返回一个指向 dirent 结构的指针，该结构表示 dirp 指向的目录流中的下一个目录条目。 它在到达末尾时返回 NULL 目录流或发生错误。类似于读文件流，会自动指向下一个。

```C
struct dirent *readdir(DIR *dirp);

int readdir_r(DIR *dirp, struct dirent *entry, struct dirent **result); // 函数名加r表示重订版

struct dirent {
   ino_t          d_ino;       /* Inode number */
   off_t          d_off;       /* Not an offset; see below */
   unsigned short d_reclen;    /* Length of this record */
   unsigned char  d_type;      /* Type of file; not supported by all filesystem types */
   char           d_name[256]; /* Null-terminated filename */
};
```

#### 12. 目录解析的具体实现，需要使用到递归, 见 mydu.c

```C
du : 可以分析出来一个目录或者是一个文件所占的磁盘空间大小，默认以 K 为单位
```

实现 mydu 其实可以用 stat 中的 block 属性，一个 block 为 512B，所以只要将 block 数除以 2 即是以 K 为单位的数量。

如果该目录下只有一个文件，它不会说这个文件对应多大，而是以 `. ---->  大小` 这样的形式，表示当前目录的总大小，包括了一些隐藏文件。

如果该目录下有目录，还包含有文件，目录下的目录还有目录或者文件，它递归是递归计算到目录。例如：

```
当前目录树
.
├── File_IO
│   ├── dup.c
│   ├── mycp_1.c
│   ├── out
│   ├── test_1.md
│   └── test.md
├── File_sys
│   ├── bigfile.c
│   ├── flen.c
│   ├── glob.c
│   ├── mydu.c
│   └── opendir.c
└── test
    └── test.c
```

du 命令显示的形式

```
4       ./test
24      ./File_IO
20      ./File_sys
52      .
```

像 File_IO 目录下的文件并没有一一列出来对应的大小，File_sys 目录下的文件也是一样，只列出来了该目录的对应大小。最后以 . 表示当前目录的所有文件、目录和隐藏文件的总大小。

文件类型 权限 硬连接数 【owner】 【group】 size 【时间】 文件名

如果一个变量的使用完全在递归点之前或之后，该变量可以放在静态区；如果该变量跨递归点使用，则只能是auto类型。

mydu.c中使用了递归，可以使用递归优化，将不参与递归的变量设置为static静态变量
