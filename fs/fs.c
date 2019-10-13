#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <dirent.h>
#include <errno.h>

#define PORT "58020"
#define MAX_TOPICS 99
#define TOPIC_SIZE 17

#define max(A, B) (A >= B ? A : B)

enum options { REG, LTP, PTP, LQU, GQU, QUS, ANS };

struct stat s = {0};
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

int dirSize(char* path) {   //VERIFICACOES

    DIR *d;
    struct dirent *dir;
    int size = 0;

    d = opendir(path);
    if (d) {
        readdir(d);
        readdir(d);
        while ((dir = readdir(d)) != NULL) {
            if (dir->d_type == DT_DIR) {
                size++;                     //CHAMADA SISTEMA?
            }
        }
        closedir(d);                                //CHAMADA SISTEMA?
        return size;
    }
    else {
        return -1;
    }
}

char* getDirContent(char* path, char* answer, int flag) {

    DIR *d;
    struct dirent *dir;

    d = opendir(path);
    if (d) {
        readdir(d);
        readdir(d);
        while ((dir = readdir(d)) != NULL) {
            if (dir->d_type == DT_DIR) {
                char fName[37], subDir[11], id[6];
                FILE *f;

                strcpy(subDir, dir->d_name);
                sprintf(fName, "%s/%s/%s_UID.txt", path, subDir, subDir);

                f = fopen(fName, "r");              //CHAMADA SISTEMA?
                sprintf(fName, "%s/%s", path, subDir);

                fgets(id, 6, f);
                if (flag) {
                    sprintf(answer, "%s %s:%s:%d", answer, subDir, id, dirSize(fName));
                }
                else {
                    sprintf(answer, "%s %s:%s", answer, subDir, id);
                }

                fclose(f);                          //CHAMADA SISTEMA?
            }
        }
        closedir(d);                                //CHAMADA SISTEMA?
    }
    return answer;
}

void createTopic(char* topic, char* id) {

    char path[37] = "";
    FILE *f;
    
    sprintf(path, "topics/%s", topic);
    mkdir(path, 0700);                          //FLAGS CORRETAS? //CHAMADA SISTEMA?

    sprintf(path, "topics/%s/%s_UID.txt", topic, topic);
    f = fopen(path, "w");                       //CHAMADA SISTEMA?
    fwrite(id, 1, strlen(id), f);
    fclose(f);                                  //CHAMADA SISTEMA?
    
    nTopics++;
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

int checkTopic(char* topic) {

    DIR *d;
    char path[18] = "";

    sprintf(path, "topics/%s", topic);

    d = opendir(path);
    if (d) { 
        closedir(d); 
        return 1; 
    }
    else if (ENOENT == errno) { return 0;}
    else { error(1); } //CHECK

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
        answer = getDirContent("topics", answer, 0);
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

    if (checkTopic(topic)) {
        strcpy(answer, "PTR DUP\n");
        free(buffer);
        return answer;
    }

    createTopic(topic, id);
    strcpy(answer, "PTR OK\n");

    free(buffer);
    return answer;
}

char* lqu(char *buffer) {

    char* topic, path[30], *answer = (char*) malloc(sizeof(char)*(MAX_TOPICS*(TOPIC_SIZE+3)+8));
    int size;

    strcpy(answer, "LQR");
    strtok(buffer, " ");
    topic = strtok(NULL, "\n");
    
    if (!checkTopic(topic)) {
        free(buffer);
        strcpy(answer, "ERR\n");
        return answer;
    }
    
    sprintf(path, "topics/%s", topic);
    free(buffer);

    size = dirSize(path);
    if (!size) {
        strcat(answer, " 0\n");
        return answer;
    }

    sprintf(answer, "%s %d", answer, size);
    answer = getDirContent(path, answer, 1);
    strcat(answer, "\n");
    return answer;    
}

char* handleMessage(char *buffer) {

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
            return lqu(buffer);
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

    int option, p = 0, fdUDP, fdTCP, fdMax, err;
    char *fsport = PORT, *buffer;
    struct addrinfo hints, *resUDP, *resTCP;
    struct sockaddr_in addr;
    socklen_t addrlen;
    ssize_t n, nread;
    fd_set rfds;
    
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

    //Initializing topic folder
    if (stat("topics", &s) == -1) {              //CHAMADA SISTEMA?
        printf("STDERR: CREATING TOPICS FOLDER\n");
        mkdir("topics", 0700);                   //CHAMADA SISTEMA?
    }
    nTopics = dirSize("topics");

    //Initializing UDP socket
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_PASSIVE|AI_NUMERICSERV;

    if ((err = getaddrinfo(NULL, fsport, &hints, &resUDP)) != 0) error(2);
    if ((fdUDP = socket(resUDP->ai_family, resUDP->ai_socktype, resUDP->ai_protocol)) == -1) error(2);
    if (bind(fdUDP, resUDP->ai_addr, resUDP->ai_addrlen) == -1) error(2);
    
    //Initializing TCP socket
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE|AI_NUMERICSERV;

    if ((err = getaddrinfo(NULL, fsport, &hints, &resTCP)) != 0) error(2);
    if ((fdTCP = socket(resTCP->ai_family, resTCP->ai_socktype, resTCP->ai_protocol)) == -1) error(2);
    if (bind(fdTCP, resTCP->ai_addr, resTCP->ai_addrlen) == -1) error(2);
    if (listen(fdTCP, 5) == -1) error(2);

    int i = 0;
    while (i != 10) { 
        /*Mask initialization*/
        FD_ZERO(&rfds);
        FD_SET(fdUDP, &rfds);
        FD_SET(fdTCP, &rfds);
        fdMax = max(fdUDP, fdTCP);

        /*Waits for commands*/
        err = select(fdMax+1, &rfds, (fd_set*) NULL, (fd_set*) NULL, (struct timeval *)NULL);
        if (err <= 0) { error(2); }

        /*Reads input from an UDP connection*/
        if (FD_ISSET(fdUDP, &rfds)) {
            buffer = readMessageUDP(buffer, fdUDP, &addr, &addrlen);
            buffer = handleMessage(buffer);
            sendMessageUDP(buffer, fdUDP, addr, addrlen);
            free(buffer);
        }

        /*Reads input from an TCP connection*/
        if (FD_ISSET(fdTCP, &rfds)) {

        }
        i++;
    }

    freeaddrinfo(resUDP);
    freeaddrinfo(resTCP);
    close(fdUDP);
    close(fdTCP);
    return 0;
}

/*
CHECKAR SE MENSAGENS TERMINAM COM UM \N NO CLIENTE
*/
