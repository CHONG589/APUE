#ifndef MYPIPE_H__
#define MYPIPE_H__

#define PIPESIZE        1024

// 这两个是一个位图，1 表示最低位为 1，2 表示次低位为 1
#define MYPIPE_READ     0x00000001UL    // unsigned long
#define MYPIPE_WRITE    0x00000002UL    // unsigned long

typedef void mypipe_t;

mypipe_t *mypipe_init(void);

// 为线程确定身份：读者/写者
// 针对第一个参数中的管道，注册第二个参数中的身份。
int mypipe_register(mypipe_t *, int opmap);

// 注销身份
int mypipt_unregister(mypipe_t *, int opmap);

/*
 * 返回读到的字节数
 */
int mypipe_read(mypipe_t *, void *buf, size_t count);

int mypipe_write(mypipe_t *, const void *buf, size_t count);

int mypipe_destroy(mypipt_t *);

#endif
