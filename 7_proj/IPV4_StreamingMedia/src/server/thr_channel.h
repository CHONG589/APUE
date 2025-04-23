#ifndef THR_CHANNEL_H__
#define THR_CHANNEL_H__

#include "medialib.h"

/**
 * 创建对应频道的线程，参数是对应频道的节目单信息
 */
int thr_channel_create(struct mlib_listentry_st *);

int thr_channel_destroy(struct mlib_listentry_st *);

/**
 * 销毁所有可以不需要对应的频道，直接销毁所有就可以。
 */
int thr_channel_destroyall(void);

#endif
