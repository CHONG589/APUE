#include <stdio.h>
#include <stdlib.h>
#include <syslog.h>
#include <errno.h>
#include <string.h>
#include <glob.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include "medialib.h"
#include "mytbf.h"
#include <site_type.h>
#include <proto.h>
#include "server_conf.h"

#define PATHSIZE        1024
#define LINEBUFSIZE     1024
#define MP3_BITRATE     (1024 * 256)//32KB/s

/**
 * 这个结构体是数据库中每个频道的详细信息，而只给
 * 用户看的（在 .h 文件中）只有频道号和描述。
 * 
 * 在使用这里提供的函数获取频道信息时，是要解析频道
 * 目录中有哪些文件，例如：一定有一个频道文件，如
 * .mp3 文件，还可以有描述文件（可有可无），所以就要
 * 去解析目录中有哪些文件，然后将对应的信息填到下面
 * 的结构体中。
 */
struct channel_context_st {
    chnid_t chnid;
    char *desc;
    glob_t mp3glob;
    // 当前频道位置。
    int pos;
    // 打开这个文件所用到的文件描述符
    int fd;
    // 记录播放的位置。
    off_t offset;
    // 流量控制
    mytbf_t *tbf;
};

static struct channel_context_st channel[MAXCHNID + 1];

static struct channel_context_st *path2entry(const char *path) {

    FILE *fp;
    char pathstr[PATHSIZE] = {'\0'};
    char linebuf[LINEBUFSIZE];
    struct channel_context_st *me;
    static chnid_t curr_id = MINCHNID;

    // 先将描述文件解析出来
    strncpy(pathstr, path, PATHSIZE);
    strncat(pathstr, "/desc.text", PATHSIZE);

    fp = fopen(pathstr, "r");
    if (fp == NULL) {
        syslog(LOG_INFO, "%s is not a channel dir(Can't find desc.text)", path);
        return NULL;
    }
    // 从 fp 中读取一行，并把它存储在 linebuf 中，当读取到 (LINEBUFSIZE - 1) 个字符
    // 时，或者读取到换行符时，或者到达文件末尾时，它会停止。
    // 但是这里这样写不太准确，这里写死了，当描述只有一行且不超过 1024 字节时，才能
    // 能行，当超过一行时，超过的部分读取不到，所以这里应该有一个循环，直到读到空行
    // 为止。
    if (fgets(linebuf, LINEBUFSIZE, fp) == NULL) {
        syslog(LOG_INFO, "[medialib][channel_context_st] %s is not a channel dir (Cannot get the desc.text)", path);
        fclose(fp);
        return NULL;
    }
    fclose(fp);
    
    // me 为最后返回的结果，现在为它分配空间。
    me = malloc(sizeof(*me));
    if (me == NULL) {
        syslog(LOG_ERR, "[medialib][channel_context_st] malloc fail : %s", strerror(errno));
        return NULL;
    }

    // me 为 channel_context_st 结构体，这时分配内存空间后就要为 me 中的
    // 元素初始化。
    me->tbf = mytbf_init(MP3_BITRATE/16, MP3_BITRATE/8*10);
    if (me->tbf == NULL) {
        syslog(LOG_ERR, "[medialib][channel_context_st] mytbf_init fail : %s", strerror(errno));
        free(me);
        return NULL;
    }
    me->desc = strdup(linebuf);

    // 解析 .mp3 文件
    strncpy(pathstr, path, PATHSIZE);
    strncat(pathstr, "/*.mp3", PATHSIZE);
    if (glob(pathstr, 0, NULL, &me->mp3glob) != 0) {
        curr_id++;
        syslog(LOG_ERR, "[medialib][channel_context_st] %s is not channel dir (cannot find mp3 files)", path);
        free(me);
        return NULL;
    }
    me->pos = 0;
    me->offset = 0;
    me->fd = open(me->mp3glob.gl_pathv[me->pos], O_RDONLY);
    if (me->fd < 0) {
        syslog(LOG_WARNING, "[medialib][channel_context_st] %s open failed", me->mp3glob.gl_pathv[me->pos]);
        free(me);
        return NULL;
    }
    me->chnid = curr_id;
    curr_id++;
    return me;
}

/**
 * 第一个参数是结构体类型的数组
 * 第二个参数是数组的长度
 * 创建成功后，会把数组存到第一个参数中，并且把长度
 * 回填到第二个参数中。
 */
int mlib_getchnlist(struct mlib_listentry_st **result, int *resnum) {

    int i;
    char path[PATHSIZE];
    int num = 0;
    // 用来装回填给用户看的结构体
    struct mlib_listentry_st *ptr;
    // 用来装频道真正的具体信息的结构体
    struct channel_context_st *res;
    glob_t globres;

    // 初始化数组
    for (i = 0; i < MAXCHNID + 1; i++) {
        channel[i].chnid = -1;
    } 

    // 测试语句，打印媒体库位置。
    snprintf(path, PATHSIZE, "%s*", server_conf.media_dir);
    syslog(LOG_INFO, "medialib path : %s", path);

    if (glob(path, 0, NULL, &globres)) {

        syslog(LOG_ERR, "[medialib][mlib_getchnlist] fail to glob path : %s", strerror(errno));
        return -1;
    }

    ptr = malloc(sizeof(struct mlib_listentry_st) * globres.gl_pathc);
    // 如果是空目录
    if (ptr == NULL) {
        syslog(LOG_ERR, "[medialib][mlib_getchnlist] malloc() error.");
        exit(1);
    }

    for (i = 0; i < globres.gl_pathc; i++) {
        // 用 glob 解析出来该目录下有多少个目录，一个目录表示
        // 一个频道。
        // 该函数是将频道目录继续解析，看频道目录中有啥文件，如：
        // .mp3 文件，或者描述文件等，即将它解析成记录。
        syslog(LOG_INFO, "globres.gl_pathv[%d] path is %s", i, globres.gl_pathv[i]);
        res = path2entry(globres.gl_pathv[i]);
        if (res != NULL) {
            syslog(LOG_INFO, "%d : %s", res->chnid, res->desc);
            // 将解析出来的频道数据存到全局变量 channel 数组中
            memcpy(channel + res->chnid, res, sizeof(*res));
            // 也要存到给用户看的结构体中
            ptr[num].chnid = res->chnid;
            ptr[num].desc = strdup(res->desc);
            num++;
        }
    }
    // 将 ptr 数组存的结构体复制到给 result 中，并为它分配空间。
    *result = realloc(ptr, sizeof(struct mlib_listentry_st) * num);
    if (*result == NULL) {
        syslog(LOG_ERR, "[medialib][mlib_getchnlist] realloc failed.");
    }
    *resnum = num;

    return 0;
}

static int open_next(chnid_t chnid) {

    int i;

    for (i = 0; i < channel[chnid].mp3glob.gl_pathc; i++) {

        channel[chnid].pos++;

        if (channel[chnid].pos == channel[chnid].mp3glob.gl_pathc) {
            // 已为该频道中的文件都尝试去打开了，绕了一圈都没有打开。
            // 重头再来
            channel[chnid].pos = 0;
            break; 
        }

        close(channel[chnid].fd);
        // mp3glob 中存的是该频道文件中有哪些文件，它是 glob_t 类型
        // 通过 glob() 解析出来，然后存到 mp3glob 中，其中 gl_pachv
        // 中存的是文件名。 
        // 本频道的结构体中的 pos 已经递增到下一个频道了，所以我们
        // 想要打开下一个频道可以利用 pos，open 是利用文件名来打开文件
        // 的。下面是通过 pos 寻找到下一个文件名的。
        // channel[chnid].pos 是下一个文件名的位置。
        channel[chnid].fd = open(channel[chnid].mp3glob.gl_pathv[channel[chnid].pos], O_RDONLY);
        if (channel[chnid].fd < 0) {
            syslog(LOG_WARNING, "open(%s) : %s", channel[chnid].mp3glob.gl_pathv[channel[chnid].pos], strerror(errno));
        }
        else {
            // successed
            // 成功打开，文件位移置 0
            channel[chnid].offset = 0;
            return 0;
        }
    }
    syslog(LOG_ERR, "None of mp3s in channel %d is available.", chnid);
}

/**
 * 第一个参数是要读取的频道，
 * 第二个参数是读取后的内容存取的地方。
 * 第三个参数是想要读取的字节个数。
 * 返回真正读取到的字节数。
 */
size_t mlib_readchn(chnid_t chnid, void *buf, size_t size) {

    int tbfsize, len;

    tbfsize = mytbf_fetchtoken(channel[chnid].tbf, size);

    while (1) {

        // pread() 可以从文件指定位置（offset)开始读
        len = pread(channel[chnid].fd, buf, tbfsize, channel[chnid].offset);
        if (len < 0) {
            // 读取失败，不一定要退出，如读取当前文件失败，我们可以读取当前
            // 频道的下一首歌曲。
            syslog(LOG_WARNING, "media file %s pread() : %s", channel[chnid].mp3glob.gl_pathv[channel[chnid].pos], strerror(errno));
            // 读取下一首歌
            open_next(chnid);
        }
        else if (len == 0) {
            // 当前这首歌播放结束
            syslog(LOG_DEBUG, "media file %s is over.", channel[chnid].mp3glob.gl_pathv[channel[chnid].pos]);
            open_next(chnid);
        }
        else {
            // 读取到内容
            // 移动读取的位置
            channel[chnid].offset += len;
            break;
        }
    }
    if (tbfsize - len > 0) {
        mytbf_returntoken(channel[chnid].tbf, tbfsize - len);
    }

    return len;
}

int mlib_freechnlist(struct mlib_listentry_st *ptr) {

    free(ptr);
    return 0;
}
