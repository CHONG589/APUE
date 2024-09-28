#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <string.h>

#define TIMESTRSIZE 1024
#define FMTSTRSIZE  1024

/**
 * %Y: year
 * %m: mouth
 * %d: date
 * %H: hour
 * %M: minute
 * %S: second
 */

int main(int argc, char **argv) {

	time_t stamp;
	struct tm *tm;
	char timestr[TIMESTRSIZE];
    int c;
    char fmtstr[FMTSTRSIZE];
    FILE *fd = stdout;

    fmtstr[0] = '\0';

	stamp = time(NULL);
    tm = localtime(&stamp);

    while (1) {
        c = getopt(argc, argv, "-H:MSy:md");
        if (c < 0) {
            break;
        }

        switch(c) {
            case 1:
                if (fd == stdout) {
                    fd = fopen(argv[optind - 1], "w");
                    if (fd == NULL) {
                        perror("fopen()");
                        fd = stdout; 
                    }
                }
                break;
            case 'H':
                if (strcmp(optarg, "12") == 0) {
                    strncat(fmtstr, "%I(%P) ", FMTSTRSIZE - 1);
                } else if (strcmp(optarg, "24") == 0) {
                    strncat(fmtstr, "%H ", FMTSTRSIZE - 1);
                } else {
                    fprintf(stderr, "Invalid argument -H!\n");
                }
                break;
            case 'M':
                strncat(fmtstr, "%M ", FMTSTRSIZE - 1);
                break;
            case 'S':
                strncat(fmtstr, "%S ", FMTSTRSIZE - 1);
                break;
            case 'y':
                if (strcmp(optarg, "2") == 0) {
                    strncat(fmtstr, "%y ", FMTSTRSIZE - 1);
                } else if (strcmp(optarg, "4") == 0) {
                    strncat(fmtstr, "%Y ", FMTSTRSIZE - 1);
                } else {
                    fprintf(stderr, "Invalid argument -y!\n");
                }
                break;
            case 'm':
                strncat(fmtstr, "%m ", FMTSTRSIZE - 1);
                break;
            case 'd':
                strncat(fmtstr, "%d ", FMTSTRSIZE - 1);
                break;
            default:
                break;
        }
    }

    strncat(fmtstr, "\n", FMTSTRSIZE - 1);
	strftime(timestr, TIMESTRSIZE, fmtstr, tm);
	fputs(timestr, fd);

    if (fd != stdout) {
        fclose(fd);
    }

	exit(0);
}
