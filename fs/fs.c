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
        while ((dir = readdir(d)) != NULL) {
            if ((dir->d_type == DT_DIR) && strcmp(dir->d_name, ".") && strcmp(dir->d_name, "..")) {
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
        while ((dir = readdir(d)) != NULL) {
            if ((dir->d_type == DT_DIR) && strcmp(dir->d_name, ".") && strcmp(dir->d_name, "..")) {
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

int checkDir(char* topic) {

    DIR *d;
    char path[18] = "";

    sprintf(path, "topics/%s", topic);
    printf("STDERR: CHECKING PATH: %s\n", path);

    d = opendir(path);
    if (d) { 
        closedir(d); 
        return 1; 
    }
    else if (ENOENT == errno) { return 0;}
    else { error(1); } //CHECK

}

int verifyTopic(char* topic) {

    if (topic != NULL && strlen(topic) <= 10) {
        for (int i = 0; i < strlen(topic); i++) {
            if ((topic[i] < '0' || topic[i] > '9') && (topic[i] < 'A' || topic[i] > 'Z') && (topic[i] < 'a' || topic[i] > 'z')) {
                return 1;
            }
        }
        return 0;
    }
    return 1; 
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

char* readMessageTCP(char* buffer, int nBytes, int fdTCP, struct sockaddr_in *addr, socklen_t *addrlen) {

    int readBytes = 0;
    char* message = (char*) malloc(sizeof(char)*(nBytes+1));

    buffer = (char*) malloc(sizeof(char)*(nBytes+1));
    memset(buffer, '\0', nBytes);

    while (readBytes < nBytes) {
        readBytes += read(fdTCP, message, nBytes);
        message[readBytes] = '\0';
        printf("%d|", readBytes);
        strcat(buffer, message);
        if (buffer[readBytes-1] = '\n') {
            break;
        }
    }

    strcpy(buffer, message);
    free(message);

    printf("\nSTDERR: | Size: %ld | data: %d | MESSAGE RECEIVED TCP: %s", strlen(buffer), readBytes, buffer);
    return buffer;
}

void sendMessageTCP(char* buffer, int size, int fdTCP) {

    int n;

    printf("STDERR: Size: %d | MESSAGE to ben sent TCP: %s\n", size, buffer);
    n = write(fdTCP, buffer, size);
    if (n == -1) error(2);
    printf("STDERR: Size: %d | MESSAGE SENT TCP: %s\n", size, buffer);
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

    if (!checkUser(id) || verifyTopic(topic)){
        free(buffer);
        return answer;
    }

    if (checkDir(topic)) {
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
    
    if (!checkDir(topic)) {
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

int readAndSendFile(char* path, char* mode, int fd) {

    FILE *f;
    int size = 0;
    char *buffer, fileSize[12]; 

    f = fopen(path, mode);

    fseek(f, 0 , SEEK_END);
    size = ftell(f);
    fseek(f, 0 , SEEK_SET);

    if (!strcmp(mode, "rb")) {
        sprintf(fileSize, " %d ", size);
        sendMessageTCP(fileSize, strlen(fileSize), fd);
    } 

    buffer = (char*) malloc(sizeof(char)*(size+1)); 
    if (fread(buffer, size, 1, f) == -1) error(2); //check error
    buffer[size] = '\0';
    
    fclose(f);

    sendMessageTCP(buffer, size, fd);
    free(buffer);

    return 0;
}

void handleFolder(char* path, char* dirName, int fd) {

    DIR *d;
    struct dirent *dir;
    int size, qIMG = 0, ansSize = 270, nBytes = 0;
    long fileSize = 0;
    char pathUID[100], pathTitle[100], ext[100], pathIMG[1000], buffer[256] = "", *answer = (char*) malloc(sizeof(char)*ansSize), *buffer2;

    sprintf(pathUID, "topics/%s/%s_UID.txt", path, dirName);
    sprintf(pathTitle, "topics/%s/%s.txt", path, dirName);
    printf("STDERR: Question INFO: pathUID: %s | pathTitle: %s | path: %s\n", pathUID, pathTitle, path);

    readAndSendFile(pathUID, "r", fd);
    readAndSendFile(pathTitle, "rb", fd);

    char att[233];
    sprintf(att, "topics/%s", path);

    d = opendir(att);
    if (d) {
        while ((dir = readdir(d)) != NULL) {
            char uid[200], title[200];
            sprintf(uid, "%s_UID.txt", dirName);
            sprintf(title, "%s.txt", dirName);
            printf("\n : %s\n", uid);
            printf("\n : %s\n", title);
                
            if ((dir->d_type != DT_DIR) && strcmp(dir->d_name, uid) && strcmp(dir->d_name, title)) {
                char d_name[256], *token;
                strcpy(d_name, dir->d_name);
                token = strtok(d_name, ".");
                token = strtok(NULL, "");
                strcpy(ext, token);
                qIMG = 1;
                sprintf(pathIMG, "topics/%s/%s", path, dir->d_name);
                printf("STDERR: topics/%s/%s\n", path, dir->d_name);
                
            }
        }
        closedir(d);                                //CHAMADA SISTEMA?
    }
    else {

    }

    if (qIMG) { 
        sendMessageTCP(" 1 ", 3, fd);
        sendMessageTCP(ext, strlen(ext), fd);
        readAndSendFile(pathIMG, "rb", fd);    
    }
    else {
        sendMessageTCP(" 0", 2, fd);
    }

}
char* dirInfo(char* path, char* dirName, int fd, struct sockaddr_in *addr, socklen_t *addrlen) {

    DIR *d;
    struct dirent *dir;
    int size, qIMG = 0, ansSize = 270, nBytes = 0;
    long fileSize = 0;
    char pathUID[100], pathTitle[100], pathIMG[1000], buffer[256] = "", *answer = (char*) malloc(sizeof(char)*ansSize), *buffer2;
    FILE *f;

    sendMessageTCP("QGR ", 4, fd);

    handleFolder(path, dirName, fd);
    
    char att[233];
    sprintf(att, "topics/%s", path);
    size = dirSize(att);
    if (size >= 10) {
        sendMessageTCP(" 10 ", 4, fd);
    }
    else if (size == 0) {
        sendMessageTCP(" 0\n", 3, fd);
    }
    else {
        sprintf(answer, " %d ", size);
        sendMessageTCP(answer, strlen(answer), fd);
    }

    for (int i = 10; i > 0 && size > 0; i--) {
        char pathAnswer[1000] = "", AN[4] = "", d_name[300] = "";
        sprintf(d_name, "%s_%02d", dirName, size);
        sprintf(pathAnswer, "%s/%s_%02d", path, dirName, size);
        sprintf(AN, "%02d ", size);
        sendMessageTCP(AN, strlen(AN), fd);
        printf("STDERR: pathanswer: %s\n", pathAnswer);
        handleFolder(pathAnswer, d_name, fd);
        size--;
        if (size && i) {
            sendMessageTCP(" ", 1, fd);
        }
        else {
            sendMessageTCP("\n", 1, fd);
        }
    }
}

char* gqu(char *buffer, int fd, struct sockaddr_in *addr, socklen_t *addrlen) {

    char *topic, *question, *answer;

    free(buffer);
    buffer = readMessageTCP(buffer, 23, fd, addr, addrlen);

    topic = strtok(buffer, " ");
    question = strtok(NULL, "\n");

    printf("STDERR: GQU: %s | %s \n", topic, question);

    if (verifyTopic(topic) || verifyTopic(question)) {
        char* answer = (char*) malloc(sizeof(char)*9);
        strcpy(answer, "QGR ERR\n");
        free(buffer);
        return answer;
    }

    char path[30];
    sprintf(path, "%s/%s", topic, question);


    if (!checkDir(topic) || !checkDir(path)) {
        char* answer = (char*) malloc(sizeof(char)*9);
        strcpy(answer, "QGR EOF\n");
        return answer;
    }

    answer = dirInfo(path, question, fd, addr, addrlen);
    free(buffer);
    return answer;

}

char* handleMessage(char *buffer, int fd, struct sockaddr_in *addr, socklen_t *addrlen) {

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
            return gqu(buffer, fd, addr, addrlen);
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
            buffer = handleMessage(buffer, fdUDP, &addr, &addrlen);
            sendMessageUDP(buffer, fdUDP, addr, addrlen);
            free(buffer);
        }

        /*Reads input from an TCP connection*/
        if (FD_ISSET(fdTCP, &rfds)) {
            int fdNew;
            addrlen = sizeof(addr);
            if ((fdNew = accept(fdTCP, (struct sockaddr*)&addr, &addrlen)) == -1) error(2); //check error code
            buffer = readMessageTCP(buffer, 3, fdNew, &addr, &addrlen);
            handleMessage(buffer, fdNew, &addr, &addrlen);
            close(fdNew);
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
