#ifndef PROTO_H__
#define PROTO_H__

#define KEYPATH     "/etc/services"
#define KEYPROJ     'a'

#define PATHMAX     1024
#define DATAMAX     1024

// 包的类型
enum {
    MSG_PATH = 1;
    MSG_DATA,
    MSG_EOT
};

// path 包，由 serve 端接收
typedef struct msg_path_st {
    long mtype;//must be MSG_PATH
    char path[PATHMAX];// ASCIIZ 带尾 0 的串
} msg_path_t;

// client 端接收
typedef struct msg_data_st {
    long mtype;// must be MSG_DATA
    char data[DATAMAX];
    int datalen;// 描述有效字符，防止有空洞文件
} msg_data_t;

// client 端接收
typedef struct msg_eot_st {
    long mtype;// must be MSG_EOT
} msg_eot_t;

// 所以 client 端会接收两种类型的包，为了区别收到的
// 是什么包，这就用到了 long mtype;

// client 端接收到的包只能两者取其一，所以用 union
union msg_s2c_un {
    long mtype;
    msg_data_t datamsg;
    msg_eot_t eotmsg;
};
// 对于 union 的灵活使用，当我们 client 在接收包时，我们是不知道它
// 具体是什么包的，不要去猜测或者说因为 msg_data_t 和 msg_eot_t 包
// 中都有 long mtype ，我们先把包默认为 msg_data_t，通过查看 mtype 
// 然后再来确定是什么包，这样是不合理的，这涉及到了强转，逻辑上是不
// 对的，解决方法就是 union 的灵活使用，里面把两种包都定义进去，同时
// 只能存在一种包，里面还有一个成员 mtype，不管是哪种包，前四个字节
// 都是 mtype，我们可以查看 union 成员 mtype 就可以知道什么包，这就
// 不会涉及到强转类型了。

#endif
