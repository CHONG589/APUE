#ifndef PROTO_H__
#define PROTO_H__

#define NAMESIZE        11

#define RCVPORT         "1989"    // 指定端口

#define MGROUP          "224.2.2.2"

struct msg_st {
    // 考虑到每个机器不同，所以用指定位数的类型
    uint8_t name[NAMESIZE];
    uint32_t math;
    uint32_t chinese;
}__attribute__((packed));// 告诉 gcc 不对齐

#endif
