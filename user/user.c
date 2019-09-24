#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

void error(int error) {
    fprintf(stdout, "ERR: Format incorrect. Should be: ./user [-n FSIP] [-p FSport]\n");
    exit(error);
}

int main(int argc, char **argv) {

    int option, n = 0, p = 0;
    char *fsip = NULL, *fsport = NULL;

    while ((option = getopt (argc, argv, "n:p:")) != -1) {
        switch (option)
        {
        case 'n':
            if (n) error(1);
            n = 1;
            fsip = optarg;
            break;
        case 'p':
            if (p) error(1);
            p = 1;
            fsport = optarg;
            break;
        default:
            break;
        }
    }

    if (optind < argc) error(1);

    printf("%s %s %i\n", fsip, fsport, optind);

    return 0;
}