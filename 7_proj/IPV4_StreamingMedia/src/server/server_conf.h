#ifndef SERVER_CONF_H__
#define SERVER_CONF_H__

#define DEFAULT_MEDIADIR            "../../media/"
#define DEFAULT_IF                  "ens33"

// 运行方式
enum {
    RUN_DAEMON = 1,//后台运行
    RUN_FOREGROUND
};

struct server_conf_st {
    char *rcvport;
    char *mgroup;
    // 媒体库相关的位置
    char *media_dir;
    // 运行模式，指定前台还是后台运行
    char runmode;
    // 指定哪个网卡发出去
    char *ifname;
};

extern struct server_conf_st server_conf;
extern int serversd;
extern struct sockaddr_in sndaddr;

#endif
