#ifndef MEDIALIB_H__
#define MEDIALIB_H__

#include <site_type.h>

struct mlib_listentry_st{
    // 频道号
    chnid_t chnid;
    // 频道描述，只要发送给 client 时不使用指针传输，只在
    // 本机中使用是可以的，要跨主机传输时再用分配内存传输。
    char *desc;
};

/**
 * 第一个参数是结构体类型的数组
 * 第二个参数是数组的长度
 * 创建成功后，会把数组存到第一个参数中，并且把长度
 * 回填到第二个参数中。
 */
int mlib_getchnlist(struct mlib_listentry_st **, int *);

/**
 * 第一个参数是要读取的频道，
 * 第二个参数是读取后的内容存取的地方。
 * 第三个参数是想要读取的字节个数。
 * 返回真正读取到的字节数。
 */
size_t mlib_readchn(chnid_t, void *, size_t);

int mlib_freechnlist(struct mlib_listentry_st *);

#endif
