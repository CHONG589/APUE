#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>

#include "mytbf.h"

#define CPS		10				// 每秒显示的字符个数
#define BUFSIZE	1024			// 通过每次读取个数来控制显示字符的个数
#define BURST	100				// 最多能攒 100 个权限，即可积攒 100 秒

int main (int argc, char **argv) {

	int src_fd;
	int des_fd = 1;
	int readnum, wrnum, position, size;
	char buf[BUFSIZE];
	mytbf_t *tbf;

	if (argc < 2) {
		
		printf("Usage: ./mytbf desfile");
		exit(1);
	}

	tbf = mytbf_init(CPS, BURST);
	if (tbf == NULL) {
		fprintf(stderr, "mytbf_init() failed!\n");
		exit(1);
	}

	do {
		src_fd = open(argv[1], O_RDONLY);
		if (src_fd < 0) {
			if (errno != EINTR) {
				perror("open()");
				exit(1);
			}
		}
	} while(src_fd < 0);

	while (1) {

		/**
		 * slowcat2.c 是用令牌的个数 token 来限制你来读的，至于
		 * 为什么要取 BUFSIZE 个，因为在取一种速率的令牌时，尽量
		 * 取多一点，要一次性能最大的使用该种速率令牌的资源才好，
		 * 所以取最多。
		 */
		size = mytbf_fetchtoken(tbf, BUFSIZE);
		if (size < 0) {
			fprintf(stderr, "mytbf_fetchtoken():%s\n", strerror(-size));
			exit(1);
		}
		
		while ((readnum = read(src_fd, buf, size)) < 0) {
			if (errno == EINTR) continue;
			perror("read()");
			break;
		}
		// 终止条件,0 表示已经读到文件末尾。
		// 文件：说明文件读完了
		// 管道或者 socket：说明对端关闭了
		if (readnum == 0) {
			break;
		}

		// 如果你读取的字符个数都还没有 token 多，那么你就要还回去，
		// 即读完就要判断以下要不要归还。
		if (size - readnum > 0) {
			mytbf_returntoken(tbf, size - readnum);
		}

		// 每次读完开始写时的初始位置为 0。
		position = 0;

		// read 返回值 > 0，返回读到的字节数，说明读取成功，那么这时就应该
		// 写入对应的文件中。
		while (readnum > 0) {
			// 如果写入成功，返回写入的字节数，但是写入的字节数有可能会
			// 小于你要求写入的字节数，因为中间过程可能会被中断打断。
			// 使之没有写完。
			wrnum = write(des_fd, buf + position, readnum);	
			if (wrnum < 0) {
				if (errno == EINTR) continue;
				perror("write()");
				// 这里允许小的内存泄露，如果用 break 也不好使，
				// 因为外层还有一层循环跳出，要实现比较麻烦
				exit(1);
			}
			// 判断有没有写完，没有写完则继续写，并将写入的位置定位到
			// 已写入字节末尾的后一个。

			// 看写的个数等不等于读到的个数，如果没有，那么继续读，即
			// readnum > 0
			readnum -= wrnum;	
			// 读到的个数没全部写完，继续写，从刚刚写完的后面继续。	
			position += wrnum;
		}
	}

	close(src_fd);
	mytbf_destroy(tbf);

	exit(0);
}
