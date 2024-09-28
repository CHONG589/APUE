#ifndef MYTBF_H__
#define MYTBF_H__

// 多线程版本

#define MYTBF_MAX       1024

typedef void mytbf_t;

/**
 * 参数分别为速率和上限。
 */
mytbf_t *mytbf_init(int cps, int burst);

int mytbf_fetchtoken(mytbf_t *, int);

int mytbf_returntoken(mytbf_t *, int);

int mytbf_destroy(mytbf_t *);

#endif
