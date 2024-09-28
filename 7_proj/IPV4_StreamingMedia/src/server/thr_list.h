#ifndef THR_LIST_H__
#define THR_LIST_H__

#include "medialib.h"

/**
 * 第一个参数是节目单，第二个是节目单的大小。
 */
int thr_list_create(struct mlib_listentry_st *, int);

// 只是对线程收尸，不需要参数。
int thr_list_destroy(void);

#endif
