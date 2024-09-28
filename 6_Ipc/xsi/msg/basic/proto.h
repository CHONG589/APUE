// 协议：约定双方对话的格式

#ifndef PROTO_H__
#define PROTO_H__

// 约定协议，ftok 中参数的内容要相同
#define KEYPATH     "/etc/services"
#define KEYPROJ     'g'   // 随便写的 

#define NAMESIZE    32

struct msg_st {
    long mtype;
    char name[NAMESIZE];// 解码格式
    int math;
    int chinese;
};

#endif
