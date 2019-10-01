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

    int option, n = 0, p = 0, userID, fdUDP, fdTCP;
    char *fsip = NULL, *fsport = NULL, command[MAXBUFFERSIZE], *token;
    int invalidUID;
    ssize_t s;
    socklen_t addrlen;
    struct addrinfo hints, *resUDP, *resTCP;
    struct sockaddr_in addr;
    char buffer[128], message_sent[30], message_received[30];

    while ((option = getopt (argc, argv, "n:p:")) != -1) {
        switch (option) {
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

    if (!n) { fsip = "localhost"; } 
    
    printf("%s %s %i\n", fsip, fsport, optind);

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_NUMERICSERV;

    s = getaddrinfo(fsip, fsport, &hints, &resUDP);
    if (s != 0) /*error*/ exit(1);

    fdUDP = socket(resUDP->ai_family, resUDP->ai_socktype, resUDP->ai_protocol);
    if (fdUDP == -1) /*error*/ exit(1);

    hints.ai_socktype = SOCK_STREAM;

    s = getaddrinfo(fsip, fsport, &hints, &resUDP);
    if (s != 0) /*error*/ exit(1);

    fdTCP = socket(resTCP->ai_family, resTCP->ai_socktype, resTCP->ai_protocol);
    if (fdTCP == -1) /*error*/ exit(1);

    while (1) {
        printf("Introduza o seu comando: ");
        // TODO substituir
        fgets(command, MAXBUFFERSIZE, stdin);

        token = strtok(command, " \n");

        if (!(strcmp(token, "register") && strcmp(token, "reg"))) {
            invalidUID = 0;
            token = strtok(NULL, " \n");
            if (strlen(token) == 5) {
                for (int i = 0; i < 5; i++) {
                    if (token[i] < '0' || token[i] > '9') {
                        printf("Introduce command in the format \"register userID\" or \"reg userID\" with userID between 00000 and 99999\n");
                        invalidUID = 1;
                        break;
                    }
                }
            }
            else
                printf("Introduce command in the format \"register userID\" or \"reg userID\" with userID between 00000 and 99999\n");

            if (!invalidUID) {
                strcat(message_sent, "REG "); strcat(message_sent, token); strcat(message_sent, "\n");
                n = sendto(fdUDP, message_sent, 10, 0, resUDP->ai_addr, resUDP->ai_addrlen);
	            if (n == -1) /*error*/ exit(1);
                
                addrlen = sizeof(addr);
                n = recvfrom(fdUDP, message_received, sizeof(message_received), 0, (struct sockaddr*) &addr, &addrlen);
                if (n == -1) /*error*/ exit(1);

                if (!strcmp(message_received, "RGR OK\n")) {
                    printf("User \"%s\" registered\n", token);
                    userID = atoi(token);
                }
                else if (!strcmp(message_received, "RGR NOK\n"))
                    printf("User \"%s\" already exists\n", token);
                else
                    /*terminar graciosamente*/
                    return 0;
            }
        }
        else if (!(strcmp(token, "topic_list") && strcmp(token, "tl")))
            return 0;
        else if (!strcmp(token, "topic_select"))
            return 0;
        else if (!strcmp(token, "ts"))
            return 0;
        else if (!(strcmp(token, "topic_propose") && strcmp(token, "tp")))
            return 0;
        else if (!(strcmp(token, "question_list") && strcmp(token, "ql")))
            return 0;
        else if (!strcmp(token, "question_get"))
            return 0;
        else if (!strcmp(token, "qg"))
            return 0;
        else if (!(strcmp(token, "question_submit") && strcmp(token, "qs")))
            return 0;
        else if (!(strcmp(token, "question_answer") && strcmp(token, "qa")))
            return 0;
        else if (!strcmp(token, "exit"))
            return 0;
        
        memset(message_sent, 0, sizeof(message_sent));
        memset(message_received, 0, sizeof(message_received));

        /*while (token != NULL) {
            printf("%s\n", token);
            token = strtok(NULL, " ");
        }*/
    }

    freeaddrinfo(resUDP);
    freeaddrinfo(resTCP);
    close(fdUDP);
    close(fdTCP);

    return 0;
}