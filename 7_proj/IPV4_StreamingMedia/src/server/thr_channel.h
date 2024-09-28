#ifndef THR_CHANNEL_H__
#define THR_CHANNEL_H__

#include "medialib.h"

/**
 * 要给每一个频道创建一个，所以要把当前的频道传输过来。 
 */
int thr_channel_create(struct mlib_listentry_st *);

int thr_channel_destroy(struct mlib_listentry_st *);

/**
 * 销毁所有可以不需要对应的频道，直接销毁所有就可以。
 */
int thr_channel_destroyall(void);

#endif
