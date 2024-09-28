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
typedef struct msg_s2c_st {
    long mtype;// must be MSG_DATA
    int datalen;// 描述有效字符，防止有空洞文件
    /**
     * datalen >  0      : datq
     *         == 0      : eot
     */
    char data[DATAMAX];
} msg_data_t;

#endif
