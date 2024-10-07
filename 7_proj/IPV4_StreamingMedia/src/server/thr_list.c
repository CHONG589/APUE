#include <stdio.h>
#include <stdlib.h>
#include <syslog.h>
#include <pthread.h>
#include <errno.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include "thr_list.h"
#include <proto.h>
#include "server_conf.h"

static pthread_t tid_list;

// 这里要另外存储穿过来的节目单，因为节目单创造完以后，在 server.c 我记得是直接销毁了
// 为节目单分配的空间的，即用完了就销毁了，所以这里要备份。
// 节目单包含的节目数量
static int nr_list_ent;
// 节目单信息数组，每一条存储一个节目频道信息
static struct mlib_listentry_st *list_ent;

static void *thr_list(void *p) {

    int totalsize, i, size, ret;
    struct msg_list_st *entlistp;
    struct msg_listentry_st *entryp;

    // 为节目单分配内存空间，节目单的结构体请看 proto.h 
    totalsize = sizeof(chnid_t);
    // nr_list_ent 为频道数目
    for(i = 0; i < nr_list_ent; i++) {
        totalsize += sizeof(struct msg_listentry_st) + strlen(list_ent[i].desc);
    }
    // 节目单包
    entlistp = malloc(totalsize);
    if (entlistp == NULL) {
        syslog(LOG_ERR, "malloc() : %s", strerror(errno));
        exit(1);
    }

    // 给节目单包中填数据
    entlistp->chnid = LISTCHNID;// 0 号包
    entryp = entlistp->entry;
    syslog(LOG_DEBUG, "nr_list_entn : %d\n", nr_list_ent);
    for (i = 0; i < nr_list_ent; i++) {
        // 为每个频道包分配空间，每个频道包的大小不一样，如音频文件不一定
        // 大小一样，描述文件不一定有
        size = sizeof(struct msg_listentry_st) + strlen(list_ent[i].desc);
        entryp->chnid = list_ent[i].chnid;
        // 该频道包的大小
        entryp->len = htons(size);
        strcpy(entryp->desc, list_ent[i].desc);
        // 填完数据后，要向后移动，移动 size 大小
        entryp = (void *)(((char *)entryp) + size);
        syslog(LOG_DEBUG, "entryp len : %d\n", entryp->len);
    }

    // 节目单包准备好后开始发送
    while (1) {
        ret = sendto(serversd, entlistp, totalsize, 0, (void *)&sndaddr, sizeof(sndaddr));
        if (ret < 0) {
            syslog(LOG_WARNING, "sendto serversd, enlistp... : %s", strerror(errno));
        }
        else {
            syslog(LOG_DEBUG, "sendto msg_list success, size is : %d", entlistp->entry->len);
        }
        sleep(1);
    }
}

/**
 * 第一个参数是节目单，第二个是节目单的大小。
 */
int thr_list_create(struct mlib_listentry_st *listp, int nr_ent) {

    int err;

    list_ent = listp;
    nr_list_ent = nr_ent;
    syslog(LOG_DEBUG, "list content : chnid : %d, desc : %s\n", listp->chnid, listp->desc);
    err = pthread_create(&tid_list, NULL, thr_list, NULL);
    if (err) {
        syslog(LOG_ERR, "pthread_create() : %s", strerror(err));
        return -1;
    }
    return 0;
}

// 只是对线程收尸，不需要参数。
int thr_list_destroy(void) {

    pthread_cancel(tid_list);
    pthread_join(tid_list, NULL);

    return 0;
}
