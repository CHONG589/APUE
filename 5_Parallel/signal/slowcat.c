#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <signal.h>
#include <sys/time.h>

#define CPS		10				// 每秒显示的字符个数
#define BUFSIZE	CPS				// 通过每次读取个数来控制显示字符的个数

static volatile int loop = 0;

static void alrm_handler(int s) {
	//alarm(1);
	loop = 1;
}

int main (int argc, char **argv) {

	struct itimerval itv;

	if (argc < 2) {
		
		printf("Usage: ./slowcat desfile");
		exit(1);
	}

	signal(SIGALRM, alrm_handler);
	// alarm(1);

	//设置周期性计时器的间隔，让它可以实现重复定时
	itv.it_interval.tv_sec = 1;
	itv.it_interval.tv_usec = 0;
	//设置初始定时时间
	itv.it_value.tv_sec = 1;
	itv.it_value.tv_usec = 0;
	if (setitimer(ITIMER_REAL, &itv, NULL) < 0) {
		perror("setitimer()");
		exit(1);
	}

	// 系统调用的描述符是 int 的，标准 IO 的才是 FILE * 
	int src_fd;
	int des_fd = 1;
	int readnum, wrnum, position;
	char buf[BUFSIZE];

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
		// 使程序阻塞，不让它执行那么快，一秒后自然会改变 loop 跳出
		// 循环，然后再执行下面程序。pause 是将阻塞程序挂起。
		while (!loop) pause();
		loop = 0;

		while ((readnum = read(src_fd, buf, BUFSIZE)) < 0) {
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
			readnum -= wrnum;
			position += wrnum;
		}
	}

	close(src_fd);

	exit(0);
}
