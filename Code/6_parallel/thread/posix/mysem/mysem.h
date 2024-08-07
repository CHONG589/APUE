#ifndef MYSEM_H__
#define MYSEM_H__

typedef void mysem_t;

mysem_t *mysem_init(int initval);

/**
 * 归还资源量
 * 将第二个参数的数量归还到第一个参数中
 */
int mysem_add(mysem_t *, int);

/**
 * 取资源量
 * 从第一个参数中的资源取第二个参数中的数量
 * 
 * 注意：如果该资源只有一个，其中一个线程需要取三个，另一个
 * 线程需要取一个，那么这时应该分配给取一个的，如果分配给取
 * 三个的，那么两个线程都要等待，这样可能会导致死锁。即要求
 * 该资源值大于等于要取的数量时，才可以分配。
 */
int mysem_sub(mysem_t *, int);

int mysem_destroy(mysem_t *);

#endif
