#ifndef CLIENT_H__
#define CLIENT_H__

// 解码器路径
// - 表示只播放标准输入来的内容
#define DEFAULT_PLAYERCMD       "/usr/bin/mpg123 - > /dev/null"

// 定义用户可以指定的标识
struct client_conf_st {
    char *rcvport;
    char *mgroup;
    char *player_cmd;
};

extern struct client_conf_st client_conf;

#endif
