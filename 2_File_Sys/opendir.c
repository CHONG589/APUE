#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <dirent.h>

#define PAT     "./"

int main() {

    DIR *dp;
    struct dirent *cur;

    dp = opendir(PAT);
    if (dp == NULL) {
        perror("opendir()");
        exit(1);
    }

    while ((cur = readdir(dp)) != NULL) {
        printf("/");
        puts(cur->d_name);
    }

    closedir(dp);


    exit(0);
}
