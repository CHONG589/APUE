#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <glob.h>
#include <string.h>

#define PATHSIZE    1024

static int path_noloop(const char *path) {
    //  /aaa/bbb/ccc/ddd/.
    // 用到的小函数：char *strrchr(const char *s, int c);
    // 指从 s 中找到最右边的 c 。
    char *pos;
    pos = strrchr(path, '/');
    if (pos == NULL) {
        exit(1);
    }
    if (strcmp(pos + 1, ".") == 0 || strcmp(pos + 1, "..") == 0 ) return 0;
    return 1;
    
}

// 返回一个 64 位的数表示目录的大小，因为大小是可能溢出的，long long
// 也是 64 位的。
// 路径可能是相对/绝对路径
// 首先目录可能是这样：/aaa/bb/cc/eee，我们是要计算 eee 下的文件或者
// 目录，所以怎么样运用递归呢？1. 遇到目录继续递归解析，2. 遇到文件计
// 算大小。
// 这里采用递归的形式解析。
static int64_t mydu(const char *path) {

    // /aaa/bb/cc/eee

    // 递归三部曲
    // 确定递归出口：解析失败，
    static struct stat statres;
    static char nextpath[PATHSIZE];
    glob_t globres;
    int err, i;
    int64_t sum;

    if (lstat(path, &statres) < 0) {
        perror("lstat()");
        exit(1);
    }
    // 用 statres.st_mode 判断当前文件类型，用里面的宏来判断
    if (!S_ISDIR(statres.st_mode)) {
        // 非目录文件，即 eee 为文件
        return statres.st_blocks;
    }

    // // 目录文件，eee 为目录，为了要解析它，后面要加上 /* 才能
    // // 解析它里面的所有文件或目录
    // strncpy(nextpath, path, PATHSIZE - 1);
    // // 在原来的内容追加 /*
    // strncat(nextpath, "/*", PATHSIZE - 1);

    // // 解析目录：/aaa/bb/cc/eee/*
    // // if err:
    // err = glob(nextpath, 0, NULL, &globres);
    // if (err != 0) {
    //     perror("glob()");
    //     exit(1);
    // }
    // sum = 0;
    // for (size_t i = 0; i < globres.gl_pathc; i++) {
    //     // 一个一个解析
    //     sum += mydu(globres.gl_pathv[i]);
    // }

    // // 解析完非隐藏文件后，还要解析隐藏文件，也就是 /.* 的文件
    // strncpy(nextpath, path, PATHSIZE - 1);
    // strncat(nextpath, "/.*", PATHSIZE - 1);
    // // 解析目录：/aaa/bb/cc/eee/.*
    // err = glob(nextpath, 0, NULL, &globres);
    // if (err != 0) {
    //     perror("glob()");
    //     exit(1);
    // }
    // for (size_t i = 0; i < globres.gl_pathc; i++) {
    //     sum += mydu(globres.gl_pathv[i]);
    // }

    memset(nextpath, 0, sizeof(nextpath));
    strncpy(nextpath, path, PATHSIZE);
    strncat(nextpath, "/*", PATHSIZE - 1);
    err = glob(nextpath, 0, NULL, &globres);
    if(err == GLOB_NOSPACE || err == GLOB_ABORTED) {
        printf("Error1 code = %d\n", err);
        exit(1);
    }

    strncpy(nextpath, path, PATHSIZE);
    strncat(nextpath, "/.*", PATHSIZE - 1);
    // 将这次解析的 /.* 的结果追加到 /* 中的结果中
    // 也就是追加到 globres 中
    err = glob(nextpath, GLOB_APPEND, NULL, &globres);
    if(err == GLOB_NOSPACE || err == GLOB_ABORTED) {
        printf("Error2 code = %d\n", err);
        exit(1);
    }

    // 当前目录也要加上去，因为目录文件也要存东西，里面有目录项。
    sum = statres.st_blocks;
    // 这里要注意以 . 或者 .. 开头的文件会使得文件系统有回路，单独去除 . 或者
    // .. 文件的时候，. 表示当前目录，.. 是指父目录，所以这就造成了回路，一直
    // 递归。
    for (i = 0; i < globres.gl_pathc; i++) {
        if (path_noloop(globres.gl_pathv[i]))
            sum += mydu(globres.gl_pathv[i]);
    }

    
    globfree(&globres);

    return sum;
}

int main(int argc, char **argv) {

    if (argc < 2) {
        fprintf(stderr, "Usage....\n");
        exit(1);
    }

    // 用自己的 mydu 函数解析目录 
    printf("%ld\n", mydu(argv[1]) / 2);

    exit(0);
}
