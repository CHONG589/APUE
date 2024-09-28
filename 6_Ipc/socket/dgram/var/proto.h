#ifndef PROTO_H__
#define PROTO_H__

// 512 是 UDP 的推荐长度，第一个 8 是报头大小，
// 第二个 8 是四字节的 math 和四字节的 chinese
#define NAMEMAX       512-8-8

#define RCVPORT         "1989"    // 指定端口

struct msg_st {
    // 考虑到每个机器不同，所以用指定位数的类型
    uint32_t math;
    uint32_t chinese;
    // 发送方的名字
    uint8_t name[1];
}__attribute__((packed));// 告诉 gcc 不对齐

#endif
