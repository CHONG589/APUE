

/**
 * 可以指定流速：CPS
 * 权限上限：BURST
 * 存放令牌数：token
 * 
 * 使用一个 token 对应一个字符
 * 
 * 一个令牌对应一个结构体，里面存有对应的 burst 等成员，然后
 * 用户在写程序的过程中，可能会用到不同速率的令牌桶，所以要有
 * 一个数组或链表来存不同速率的令牌桶，要使用时，就拿对应速率
 * 的令牌桶。所以要实现的操作有初始化，取令牌桶，没用完就归还
 * 的操作，还有销毁操作。
 */

#ifndef MYTBF_H__
#define MYTBF_H__

#define MYTBF_MAX   1024        // 能存 1024 个不同速率的令牌桶

typedef void mytbf_t;

mytbf_t *mytbf_init(int cps, int burst);

// 取令牌，从第一个参数中取第二个参数的数量个令牌，将实际取得的数目
// 通过返回值返回来。
int mytbf_fetchtoken(mytbf_t *, int);

// 归还令牌，取多了，要归还回去。
int mytbf_returntoken(mytbf_t *, int);

/**
 * init 函数一定会申请空间，申请结构体大小，所以一定要自定义
 * 销毁空间，释放掉。
 */
int mytbf_destroy(mytbf_t *);

#endif
