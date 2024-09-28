#include <stdio.h>
#include <stdlib.h>
#include <glob.h>

#define PAT     "/etc/a*.conf"

static int errfunc_(const char *errpath, int eerrno) {
    puts(errpath);
    fprintf(stderr, "ERROR MSG: %d\n", eerrno);
    return 0;
}

int main() {

    int err;
    glob_t globres;

    err = glob(PAT, 0, errfunc_, &globres);
    if (err) {
        printf("ERROR CODE = %d\n", err);
        exit(1);
    }

    for (int i = 0; i < globres.gl_pathc; i++) {
        puts(globres.gl_pathv[i]);
    }

    globfree(&globres);

    exit(0);
}
