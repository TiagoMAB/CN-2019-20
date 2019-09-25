#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <errno.h>

#define PORT "58020"
#define MAXBUFFERSIZE 4096

extern int errno;

void error(int error) {
    fprintf(stdout, "ERR: Format incorrect. Should be: ./user [-n FSIP] [-p FSport]\n");
    exit(error);
}

int main(int argc, char **argv) {

    int option, n = 0, p = 0;
    char *fsip = NULL, *fsport = NULL, command[MAXBUFFERSIZE], *token;
    int fdUDP, fdTCP;
    ssize_t s;
    socklen_t addrlen;
    struct addrinfo hints, *resUDP, *resTCP;
    char buffer[128];

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

    if (!p) { fsport = PORT; }

    if (!n) { fsip = "192.168.1.7"; } 
    
    printf("%s %s %i\n", fsip, fsport, optind);

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_NUMERICSERV;

    s = getaddrinfo(NULL, PORT, &hints, &resUDP);
    if (s != 0) /*error*/ exit(1);

    fdUDP = socket(resUDP->ai_family, resUDP->ai_socktype, resUDP->ai_protocol);
    if (fdUDP == -1) /*error*/ exit(1);

    hints.ai_socktype = SOCK_STREAM;

    if (gethostname(buffer, 128) == -1)
        fprintf(stderr, "error: %s\n", strerror(errno));
    else
        printf("host name: %s\n", buffer);
    
    s = getaddrinfo(buffer, PORT, &hints, &resTCP);
    if (s != 0) /*error*/ exit(1);

    fdTCP = socket(resTCP->ai_family, resTCP->ai_socktype, resTCP->ai_protocol);
    if (fdTCP == -1) /*error*/ exit(1);

    printf("Introduza o seu comando: ");
    // TODO substituir
    fgets(command, MAXBUFFERSIZE, stdin);

    token = strtok(command, " ");

    while (token != NULL) {
        printf("%s\n", token);
        token = strtok(NULL, " ");
    }

    freeaddrinfo(resUDP);
    freeaddrinfo(resTCP);
    close(fdUDP);
    close(fdTCP);

    return 0;
}