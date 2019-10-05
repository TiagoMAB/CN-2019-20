#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#define PORT "58020"

void error(int error) {
    fprintf(stdout, "ERR: Format incorrect. Should be: ./FS [-p FSport]\n");
    exit(error);
}

int main(int argc, char **argv) {

    int option, p = 0, running = 1;
    char *fsport = NULL;
    
    while ((option = getopt (argc, argv, "p:")) != -1) {
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

    while (running) {

    }



    return 0;
}