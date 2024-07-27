#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <glob.h>
#include <sys/types.h>
#include <sys/wait.h>

#define DELIMS      " \n\t"

struct cmd_st {
    glob_t globres;
};

static void prompt(void) {
    printf("mysh-0.1$ ");
}

static void parse(char *line, struct cmd_st *res) {

    char *tok;
    int i = 0;

    while (1) {
        tok = strsep(&line, DELIMS);
        if (tok == NULL) break;
        // 如果多个分隔符连续在一起时，它就会把
        // 第二个或者后面的分隔符解析出来，此时是
        // 没有内容的，为空，即 tok[0] == '\n'，
        // 然后继续往后面解析。
        if (tok[0] == '\0') continue;
        
        // 要有一个存储结构，注意元素的个数，是不确定的
        // 和 argv 很像的存储结构有 glob 中的 glob_t类型。
        // 而 glob 是解析通配符的(如目录路径中有 *)，而这里
        // 我们不需要你去解析，只需要你的存储结构，所以
        // 该怎么用？glob 中有一个参数 flag ，它有一个宏
        //  GLOB_NOCHECK:
        // If  no  pattern matches, return the original pattern.  By
        // default, glob() returns  GLOB_NOMATCH  if  there  are  no
        // matches. 即当没有传入通配符时，它就给你返回原串(pattern)，刚
        // 好是我们需要的。
        glob(tok, GLOB_NOCHECK | GLOB_APPEND * i, NULL, &(res->globres));
        // 这里每次将内容追加到 globres 中，但是第一次不能追加，后面的才能。
        // 所以这里用 i 控制第一次不追加。因为 globres 是 auto 类型，定义后
        // 是一个随机类型，所以第一次不能追加，必须覆盖。
        i = 1;
    }
}

int main() {

    pid_t pid;
    // 下面两个必须初始化，详见 getline 讲解
    char *linebuf = NULL;
    size_t linebuf_size = 0;
    struct cmd_st cmd;

	while(1) {

		prompt();               // 打印提示符

        // 注意：明显不是用 agrv 来传参数的，因为写的是 shell ，./mysh 时表示我要用
        // 我写的这个 shell，后面是不用带参数的，直接 ./mysh 就转换到我的 shell，那么
        // 在运行我的 shell 后怎么来知道你输入了什么参数呢？答案就是用标准输入 stdin。
        // argv 中识别的是 ./mysh 和后面的参数。
		if (getline(&linebuf, &linebuf_size, stdin) < 0) {            // 捕获输入行
            break;    
        }

		parse(linebuf, &cmd);                // 解析命令

		// 分内部和外部命令
		if(0) {                 // 内部命令

		}
		else {                  // 外部命令
            pid = fork();
            if (pid < 0) {
                perror("fork()");
                exit(1);
            }
            if (pid == 0) {
                execvp(cmd.globres.gl_pathv[0], cmd.globres.gl_pathv);
                perror("execp()");
                exit(1);
            }
            else {
                wait(NULL);
            }
		}
	}
	exit(0);
}
