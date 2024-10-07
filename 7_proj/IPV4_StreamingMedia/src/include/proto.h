#ifndef PROTO_H__
#define PROTO_H__

#include "site_type.h"

// 多播组
#define DEFAULT_MGROUP          "224.2.2.2"
#define DEFAULT_RCVPORT         "1989"

//-------------------------------------------------------------------

// UDP 包最大推荐长度，20 为 IP 包的报头，8 为 UDP 包的报头
#define MSG_CHANNEL_MAX         (65536-20-8)
// 数据包的最大值
#define MAX_DATA                (MSG_CHANNEL_MAX-sizeof(chnid_t))

// 负责往外发送数据的包
struct msg_channel_st {
    chnid_t chnid;// 类型用多大的，取决于频道数目
    uint8_t data[1];// channel 包大小，变长
} __attribute__((packed));

//-------------------------------------------------------------------

// 节目单中一个频道包的大小
struct msg_listentry_st {
    chnid_t chnid;// 描述哪个频道
    // 记录当前包的大小，用的时候才知道包的界限
    uint16_t len;
    uint8_t desc[1];// 节目单的描述也是变长的
} __attribute__((packed));

//-------------------------------------------------------------------

// 节目单的包
/**
 * 节目单形式
 * 频道 1. music: xxxxxxxxxxxxxxxxxxx
 * 频道 2. sport: xxxxxxxxxxxxxxxxxxxxxxx
 * 频道 3. xxxx: xxxxxxxxxxxxxxx
 * 频道 4. xxxxxxx: xxxxxxxxxxxxxxxxxxxxxxxxxxx
 */
// 频道个数
#define CHNNR                   100
// 0 号频道来发节目单
#define LISTCHNID               0
// 最小频道号用来发数据的是 1 号
#define MINCHNID                1
#define MAXCHNID                (MINCHNID+CHNNR-1)

#define MSG_LIST_MAX            (65536-20-8)
#define MAX_ENTRY               (MSG_LIST_MAX-sizeof(chnid_t))

struct msg_list_st {
    chnid_t chnid;      // must be LISTCHNID，即为节目单频道号
    struct msg_listentry_st entry[1];// 变长的节目单
} __attribute__((packed));

#endif
