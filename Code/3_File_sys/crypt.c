#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <shadow.h>
#include <crypt.h>
#include <string.h>

int main(int argc, char **argv) {

    char *input_pass;
    struct spwd *shadowline;
    char *crypted_pwd;
    
    if (argc < 2) {
        fprintf(stderr, "Usage...\n");
        exit(1);
    }

    input_pass = getpass("Enter Passwd: ");

    shadowline = getspnam(argv[1]);

    crypted_pwd = crypt(input_pass, shadowline->sp_pwdp);

    if (strcmp(crypted_pwd, shadowline->sp_pwdp) == 0) {
        printf("Password succ.\n");
    } else {
        printf("Password fail.\n");
    }

    exit(0);
}
