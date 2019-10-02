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
#define MESSAGE_SIZE 30

extern int errno;

enum options {Register, Topic_list, Topic_propose,
              Question_list, Question_submit,
              Question_answer, Topic_select, Ts, Question_get, Qg, Exit};

void error(int error) {
    fprintf(stdout, "ERR: Format incorrect. Should be: ./user [-n FSIP] [-p FSport]\n");
    exit(error);
}

int command_strcmp(char *token) {

    char *opts[] = {"register", "reg", "topic_list", "tl", "topic_propose", 
                    "tp", "question_list", "ql", "question_submit", "qs", "question_answer", 
                    "qa", "topic_select", "ts", "question_get", "qg", "exit"};
    int i = 0;

    for (i = 0; i < 12; i++) {
        if (!strcmp(token, opts[i])) {
            return i / 2;
        }
    }

    for(; i < 17; i++) {
        if (!strcmp(token, opts[i])) {
            return i - 6;
        }   
    }
    return i;
}

int main(int argc, char **argv) {

    int option, n = 0, p = 0, userID, fdUDP, fdTCP;
    char *fsip = "localhost", *fsport = PORT, command[MAXBUFFERSIZE] = "", *token = NULL;
    int invalidUID, result_strcmp = -1;
    ssize_t s;
    socklen_t addrlen;
    struct addrinfo hints, *resUDP, *resTCP;
    struct sockaddr_in addr;
    char buffer[128], message_sent[MESSAGE_SIZE], message_received[MESSAGE_SIZE];

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
    
    printf("STDERR: %s %s %i\n", fsip, fsport, optind);

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_NUMERICSERV;

    s = getaddrinfo(fsip, fsport, &hints, &resUDP);
    if (s != 0) /*error*/ exit(1);

    fdUDP = socket(resUDP->ai_family, resUDP->ai_socktype, resUDP->ai_protocol);
    if (fdUDP == -1) /*error*/ exit(1);

    hints.ai_socktype = SOCK_STREAM;

    s = getaddrinfo(fsip, fsport, &hints, &resTCP);
    if (s != 0) /*error*/ exit(1);

    fdTCP = socket(resTCP->ai_family, resTCP->ai_socktype, resTCP->ai_protocol);
    if (fdTCP == -1) /*error*/ exit(1);

    while (result_strcmp != Exit) {
        result_strcmp = -1;
        printf("Write command: ");

          // TODO substituir
        fgets(command, MAXBUFFERSIZE, stdin);
        token = strtok(command, " \n"); 

        if (token != NULL) {
            result_strcmp = command_strcmp(token);
        }

        switch (result_strcmp) {
            case Register:
                invalidUID = 0;
                token = strtok(NULL, "\n");

                if (token != NULL && strlen(token) == 5) {
                    for (int i = 0; i < 5; i++) {
                        if (token[i] < '0' || token[i] > '9') {
                            invalidUID = 1;
                            printf("ERR: Format incorrect. Should be: \"register userID\" or \"reg userID\" with userID between 00000 and 99999\n");
                            break;
                        }
                    }
                }
                else {
                    invalidUID = 1;
                    printf("ERR: Format incorrect. Should be: \"register userID\" or \"reg userID\" with userID between 00000 and 99999\n");
                }

                if (!invalidUID) {
                    strcat(message_sent, "REG "); strcat(message_sent, token); strcat(message_sent, "\n");
                    printf("STDERR: %s", message_sent);

                    // mensagem vai toda de uma vez - aula 1/10
                    n = sendto(fdUDP, message_sent, strlen(message_sent), 0, resUDP->ai_addr, resUDP->ai_addrlen);
                    if (n == -1) /*error*/ exit(1);
                    
                    addrlen = sizeof(addr);
                    n = recvfrom(fdUDP, message_received, MESSAGE_SIZE, 0, (struct sockaddr*) &addr, &addrlen);
                    if (n == -1) /*error*/ exit(1);
                    printf("STDERR: %s", message_received);

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
                break;
            case Topic_list:
                break;
            case Topic_select:
                break;
            case Ts:
                break;
            case Topic_propose:
                break;
            case Question_list:
                break;
            case Question_get:
                break;
            case Qg:
                break;
            case Question_submit:
                break;
            case Question_answer:
                break;
            case Exit:
                break;
            default:
                fprintf(stdout, "Command does not exist\n");
                break;
        }

        memset(message_sent, 0, MESSAGE_SIZE);
        memset(message_received, 0, MESSAGE_SIZE);

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