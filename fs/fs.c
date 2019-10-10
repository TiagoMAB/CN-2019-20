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
#define MAX_TOPICS 99
#define TOPIC_SIZE 17

enum options { REG, LTP, PTP, LQU, GQU, QUS, ANS };

char** tList;
int nTopics;

void error(int error) {

    switch (error) {
    case 1:
        fprintf(stdout, "ERR: Format incorrect. Should be: ./FS [-p FSport]\n");
        exit(error);
    
    case 2:
        fprintf(stdout, "ERR: Something went wrong with UDP or TCP connection\n");
        exit(error);

    case 3:
        fprintf(stdout, "ERR: Message too big, data loss expected\n");
        exit(error);

    default:
        break;
    }
}

int checkUser(char* id) {

    if (id != NULL && strlen(id) == 5) {
        for (int i = 0; i < 5; i++) {
            if (id[i] < '0' || id[i] > '9') {
                return 0;
            }
        }
        return 1;
    }
    return 0;
}

char* readMessageUDP(char* buffer, int fdUDP, struct sockaddr_in *addr, socklen_t *addrlen) {

    int size = 4, data;

    buffer = (char*) malloc(sizeof(char)*size);
    *addrlen = sizeof(addr);
    printf("STDERR: Buffer progression: %d", size);

    do {
        free(buffer);
        size *= 2;
        printf("  %d", size);
        
        buffer = (char*) malloc(sizeof(char)*size);
        data = recvfrom(fdUDP, buffer, size - 1, MSG_PEEK, (struct sockaddr*)addr, addrlen);
        buffer[size-1] = '\0'; 
    } while (strlen(buffer) == size - 1);

    free(buffer);
    buffer = (char*) malloc(sizeof(char)*(data+1));
    data = recvfrom(fdUDP, buffer, data, 0, (struct sockaddr*)addr, addrlen);
    buffer[data] = '\0';

    printf("\nSTDERR: | Size: %ld | data: %d | MESSAGE RECEIVED: %s", strlen(buffer), data, buffer);
    return buffer;
}

void sendMessageUDP(char* buffer, int fdUDP, struct sockaddr_in addr, socklen_t addrlen) {

    int n;
    addrlen = sizeof(addr);
    n = sendto(fdUDP, buffer, strlen(buffer), 0, (struct sockaddr*)&addr, addrlen);
    if (n == -1) error(2);
    printf("STDERR: Size: %ld | MESSAGE SENT: %s", strlen(buffer), buffer);
}

int checkProtocol(char* token) {

    char *opts[] = { "REG", "LTP", "PTP", "LQU", "GQU", "QUS", "ANS" };
    int i;

    for (i = 0; i < 7; i++) {
        if (!strcmp(token, opts[i])) {
            return i;
        }
    }
    return i;
}

char* reg(char* buffer) {

    char* id;
    char* answer = (char*)malloc(sizeof(char)*9);

    strcpy(answer, "RGR NOK\n");
    strtok(buffer, " ");    
    id = strtok(NULL, "\n"); 
    
    if (checkUser(id)) {
        strcpy(answer, "RGR OK\n");
    }

    free(buffer);
    return answer;
}

char* ltp(char* buffer) {

    char* answer = (char*)malloc(sizeof(char)*((TOPIC_SIZE)*(nTopics)+8));
    strcpy(answer, "ERR\n");

    if (strlen(buffer) == 4) {
        sprintf(answer, "LTR %d", nTopics);
        for (int i = 0; i < nTopics; i++) {
            strcat(answer, " "); strcat(answer, tList[i]);
        }
        strcat(answer, "\n");
    }

    free(buffer);
    return answer;
}

char* ptp(char* buffer) {

    char *id, *topic;
    char* answer = (char*)malloc(sizeof(char)*9);
    
    strcpy(answer, "PTR NOK\n");
    strtok(buffer, " ");
    id = strtok(NULL, " ");
    topic = strtok(NULL, "\n");

    if (nTopics == MAX_TOPICS) {
        free(buffer);
        strcpy(answer, "PTR FUL\n");
        return answer;       
    }

    if (!checkUser(id) || topic == NULL || strlen(topic) > 10){
        free(buffer);
        return answer;
    }

    for (int i = 0; i < nTopics; i++) {
        char current[TOPIC_SIZE];
        strcpy(current, tList[i]);
        if (!strcmp(strtok(current, ":"), topic)) {
            strcpy(answer, "PTR DUP\n");
            free(buffer);
            return answer;
        }
        
    }

    tList[nTopics] = (char*) malloc(sizeof(char)*TOPIC_SIZE);
    sprintf(tList[nTopics++], "%s:%s", topic, id);
    strcpy(answer, "PTR OK\n");

    free(buffer);
    return answer;
}

char* handleMessage(char* buffer) {

    char* message = (char*) malloc(sizeof(char)*(1+strlen(buffer)));
    char* token = NULL;
    int result = -1;

    strcpy(message, buffer);
    token = strtok(message, " \n"); 

    if (token != NULL) {
        result = checkProtocol(token);
    }
    free(message);

    switch (result) {
        case REG:
            return reg(buffer);
            break;

        case LTP:
            return ltp(buffer);
            break;

        case PTP:
            return ptp(buffer);
            break;

        case LQU:
        //    return lqu();
            break;

        case GQU:
        //    return gqu();
            break;

        case QUS:
        //    return qus();
            break;

        case ANS:
        //    return ans();
            break;

        default:
            free(buffer);
            buffer = (char*) malloc(sizeof(char)*5);
            strcpy(buffer, "ERR\n");
            return buffer;
            break;
    }
}

int main(int argc, char **argv) {

    int option, p = 0, fdUDP, fdTCP, err;
    char *fsport = PORT, *buffer;
    struct addrinfo hints, *resUDP, *resTCP;
    struct sockaddr_in addr;
    socklen_t addrlen;
    ssize_t n, nread;
    
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
    printf("%d, %d\n", optind, argc);

    if (optind < argc) error(1);

    //Initializing topic list
    tList = (char**)malloc(sizeof(char*)*MAX_TOPICS);
    nTopics = 0;

    //Initializing UDP socket
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_PASSIVE|AI_NUMERICSERV;

    if ((err = getaddrinfo(NULL, fsport, &hints, &resUDP)) != 0) error(2);
    if ((fdUDP = socket(resUDP->ai_family, resUDP->ai_socktype, resUDP->ai_protocol)) == -1) error(2);
    if (bind(fdUDP, resUDP->ai_addr, resUDP->ai_addrlen) == -1) error(2);
    int i = 0;
    while (i != 27) { 
        buffer = readMessageUDP(buffer, fdUDP, &addr, &addrlen);
        buffer = handleMessage(buffer);
        sendMessageUDP(buffer, fdUDP, addr, addrlen);
        free(buffer);
        i++;
    }

    for (int i = 0; i < nTopics; i++) {
        free(tList[i]); 
    }
    free(tList);
    freeaddrinfo(resUDP);
    close(fdUDP);
    return 0;
}

/*
CHECKAR SE TERMINA COM UM \N NO CLIENTE
*/
