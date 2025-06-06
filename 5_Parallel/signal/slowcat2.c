#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <signal.h>

#define CPS		10				// 每秒显示的字符个数
#define BUFSIZE	CPS				// 通过每次读取个数来控制显示字符的个数
#define BURST	100				// 最多能攒 100 个权限，即可积攒 100 秒

static volatile sig_atomic_t token = 0;

static void alrm_handler(int s) {
	alarm(1);
	token++;
	if (token > BURST) token = BURST;
}

int main (int argc, char **argv) {

	if (argc < 2) {
		
		printf("Usage: ./mycp_1 desfile");
		exit(1);
	}

	signal(SIGALRM, alrm_handler);
	alarm(1);

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
		while (token <= 0) pause();
		token--;

		// 如果没有数据读取时，会一直在下面的循环中转，那么每一秒都会 token++，
		// 那时也不会 token--，这样就实现了积攒权限，一秒 token 就加一个，如果
		// 有数据，加了后又会减，相当于 token 不变

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
			// 判断有没有写完，如果此次读取的数据没有一次写完，那么还要继续循环写，
			// 并将写入的位置定位到已写入字节末尾的后一个。
			readnum -= wrnum;
			position += wrnum;
		}
	}

	close(src_fd);

	exit(0);
}
