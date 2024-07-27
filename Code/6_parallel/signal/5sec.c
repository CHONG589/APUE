#include <stdio.h>
#include <stdlib.h>
#include <time.h>

int main() {

    time_t end;
    int64_t count = 0;

    // 定时 5s，获取 5s 后的时间戳，即结束的时间
    end = time(NULL) + 5;

    while (time(NULL) <= end)
        count++;

    // 定时结束后打印 count
    printf("%ld\n", count);

    exit(0);
}
