#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

void error(int error) {
    fprintf(stdout, "ERR: Format incorrect. Should be: ./user [-n FSIP] [-p FSport]\n");
    exit(error);
}

int main(int argc, char **argv) {

    int option, p = 0;
    char *fsport = NULL;
    
    while ((option = getopt (argc, argv, "n:p:")) != -1) {
        switch (option)
        {
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

    printf("%s %i\n", fsport, optind);

    return 0;
}