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
#include "../util/util.h"

#define PORT "58020"
#define MAX_TOPICS 99
#define TOPIC_SIZE 17
#define BUFFER_SIZE 2048
#define MAX_PATH_SIZE 57

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

int checkDir(char* path) {

    DIR *d;
    printf("STDERR: CHECKING PATH: %s\n", path);

    d = opendir(path);
    if (d) { 
        closedir(d); 
        return 1; 
    }
    else if (ENOENT == errno) { return 0;}
    else { exit(1); } //CHECK

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

    char path[MAX_PATH_SIZE] = "";
    FILE *f;
    
    sprintf(path, "topics/%s", topic);
    mkdir(path, 0777);                          //FLAGS CORRETAS? //CHAMADA SISTEMA?

    sprintf(path, "topics/%s/%s_UID.txt", topic, topic);
    f = fopen(path, "w");                       //CHAMADA SISTEMA?
    fwrite(id, 1, strlen(id), f);
    fclose(f);                                  //CHAMADA SISTEMA?
    
    nTopics++;
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

    if (!checkUser(id) || verifyName(topic)) {
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

    char* topic, path[MAX_PATH_SIZE] = "", *answer = (char*) malloc(sizeof(char)*(MAX_TOPICS*(TOPIC_SIZE+3)+8));
    int size;

    strcpy(answer, "LQR");
    strtok(buffer, " ");
    topic = strtok(NULL, "\n");
    printf("%s\n", topic);
    sprintf(path, "topics/%s", topic);
    printf("%s\n", path);
    free(buffer);

    if (!checkDir(path)) {
        strcpy(answer, "ERR\n");
        return answer;
    }

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

void handleFolder(char* path, char* dirName, int fd) {

    DIR *d;
    struct dirent *dir;
    int size, qIMG = 0, ansSize = 270, nBytes = 0;
    long fileSize = 0;
    char pathUID[MAX_PATH_SIZE], pathTitle[MAX_PATH_SIZE], ext[4], pathIMG[MAX_PATH_SIZE], buffer[256] = "", *answer = (char*) malloc(sizeof(char)*ansSize), *buffer2;

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
                char d_name[20], *token;
                strcpy(d_name, dir->d_name);
                token = strtok(d_name, ".");
                token = strtok(NULL, "");
                strcpy(ext, token);
                qIMG = 1;
                sprintf(pathIMG, "topics/%s/%s.%s", path, d_name, ext);
                printf("STDERR: %s\n",pathIMG);
                
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
void dirInfo(char* path, char* dirName, int fd, struct sockaddr_in *addr, socklen_t *addrlen) {

    DIR *d;
    struct dirent *dir;
    int size, qIMG = 0, ansSize = 270, nBytes = 0;
    long fileSize = 0;
    char pathUID[MAX_PATH_SIZE], pathTitle[MAX_PATH_SIZE], pathIMG[MAX_PATH_SIZE], buffer[256] = "", *answer = (char*) malloc(sizeof(char)*ansSize), *buffer2;
    FILE *f;

    sendMessageTCP("QGR ", 4, fd);

    handleFolder(path, dirName, fd);
    
    char att[233];
    printf("STDERR: path handle: %s\n", path);
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

char* gqu(int fd, struct sockaddr_in *addr, socklen_t *addrlen) {

    char *topic = readToken(topic, fd, 0);
    char *question = readToken(topic, fd, 0); 
    char *answer;

    printf("STDERR: GQU: %s | %s \n", topic, question);

    if (verifyName(topic) || verifyName(question)) {
        printf("here?");
        sendMessageTCP("QGR ERR\n", 8, fd);
        return NULL;
    }

    char path[MAX_PATH_SIZE];
    sprintf(path, "%s/%s", topic, question);

    printf("STDERR: Path: %s\n", path);
    if (!checkDir(topic) || !checkDir(path)) {
        sendMessageTCP("QGR EOF\n", 8, fd);
        return NULL;
    }

    dirInfo(path, question, fd, addr, addrlen);
    return NULL;

}

int saveFolder(int fd, char* user, char* question, char* path, struct sockaddr_in *addr, socklen_t *addrlen) {

    FILE *f;
    char pathUID[100], pathTitle[100], *fileSize, *content, *ext, *imgSize;
    int size;

    mkdir(path, 0777);                          //FLAGS CORRETAS? //CHAMADA SISTEMA?

    sprintf(pathUID, "%s/%s_UID.txt", path, question);
    printf("STDERR: SAVEFOLDER: pathuid: %s\n", pathUID);
    f = fopen(pathUID, "w");                       //CHAMADA SISTEMA?
    fwrite(user, 1, strlen(user), f);
    fclose(f);      

    sprintf(pathTitle, "%s/%s.txt", path, question);
    if (readAndWrite(pathTitle, "w", 0, fd)) {
        return 1;
    }

    readToken(NULL, fd, 0);
    content = readToken(content, fd, 1);
    printf("content: %s\n", content);

    if (!strcmp(content, "1")) {
        char pathIMG[MAX_PATH_SIZE] = "";
        ext = readToken(ext, fd, 0);
        sprintf(pathIMG, "%s/%s.%s", path, question, ext);
        if (readAndWrite(pathIMG, "wb", 0, fd)) {
            return 1;
        }
    }
    else if (!strcmp(content, "0\n")){
        return 0;
    }
    else {
        return 1;
    }

    free(content);
    content = readToken(content, fd, 1);
    if (!strcmp(content, "\n")){
        printf("STDERR: QUS: Message ended with newline character as predicted\n");
        free(content);
        return 0;
    }

    return 1;

}

void qus(int fd, struct sockaddr_in *addr, socklen_t *addrlen) {

    char path[MAX_PATH_SIZE], *topic, *question, *user;
        
    user = readToken(user, fd, 1);
    if (user[strlen(user)-1] == '\n') {
        sendMessageTCP("QUR NOK\n", 8, fd);
        return;
    }

    topic = readToken(topic, fd, 1);
    if (topic[strlen(topic)-1] == '\n') {
        sendMessageTCP("QUR NOK\n", 8, fd);
        return;
    }

    question = readToken(question, fd, 1);
    if (question[strlen(question)-1] == '\n') {
        sendMessageTCP("QUR NOK\n", 8, fd);
        return;
    }

    printf("QUS: INFO: %s | %s | %s\n", user, topic, question);
    sprintf(path, "topics/%s/%s", topic, question);
    printf("STDERR: QUS: Path: %s", path);

    if (!checkUser(user) || verifyName(topic) || verifyName(question)) {
        sendMessageTCP("QUR NOK\n", 8, fd);
    }
    else if (checkDir(path)) {
        sendMessageTCP("QUR DUP\n", 8, fd);
    }
    else if (dirSize(topic) == MAX_TOPICS) {
        sendMessageTCP("QUR FUL\n", 8, fd);
    }
    else if (saveFolder(fd, user, question, path, addr, addrlen)) {
        sendMessageTCP("QUR NOK\n", 8, fd);
    }
    else {
        printf("The user: %s has submitted a question named: %s\n", user, question);
        sendMessageTCP("QUR OK\n", 8, fd);
    }
}


void ans(int fd, struct sockaddr_in *addr, socklen_t *addrlen) {

    char path[MAX_PATH_SIZE], *topic, *question, *user;
        
    user = readToken(user, fd, 1);
    if (user[strlen(user)-1] == '\n') {
        sendMessageTCP("QUR NOK\n", 8, fd);
        return;
    }

    topic = readToken(topic, fd, 1);
    if (topic[strlen(topic)-1] == '\n') {
        sendMessageTCP("QUR NOK\n", 8, fd);
        return;
    }

    question = readToken(question, fd, 1);
    if (question[strlen(question)-1] == '\n') {
        sendMessageTCP("QUR NOK\n", 8, fd);
        return;
    }

    printf("QUS: INFO: %s | %s | %s\n", user, topic, question);
    sprintf(path, "topics/%s/%s", topic, question);
    printf("STDERR: QUS: Path: %s", path);

    if (!checkUser(user) || verifyName(topic) || verifyName(question)) {
        sendMessageTCP("QUR NOK\n", 8, fd);
    }
    else if (dirSize(question) == MAX_TOPICS) {
        sendMessageTCP("QUR FUL\n", 8, fd);
    }
    else if (saveFolder(fd, user, question, path, addr, addrlen)) {
        sendMessageTCP("QUR NOK\n", 8, fd);
    }
    else {
        printf("The user: %s has submitted an answer to the question named: %s\n", user, question);
        sendMessageTCP("QUR OK\n", 8, fd);
    }
}










void handleMessageTCP(int fd) {

    char* message;
    char* token = NULL;
    int result = -1, client;
    struct sockaddr_in addr;
    socklen_t addrlen;

    addrlen = sizeof(addr);
    if ((client = accept(fd, (struct sockaddr*)&addr, &addrlen)) == -1) error(2); //check error code
    
    message = readToken(message, client, 1);

    if (message != NULL) {
        result = checkProtocol(message);
    }
    free(message);

    switch (result) {
        case GQU:
            gqu(client, &addr, &addrlen);
            break;

        case QUS:
            qus(client, &addr, &addrlen);
            break;

        case ANS:
            ans(client, &addr, &addrlen);
            break;

        default:
            printf("Unexpected protocol message received, notifying client.\n");
            sendMessageTCP("ERR\n", 4, client);
            break;
    }

    close(client);
}

void handleMessageUDP(int fd) {

    char* message, *buffer;
    char* token = NULL;
    int result = -1;
    struct sockaddr_in addr;
    socklen_t addrlen;

    buffer = readMessageUDP(buffer, fd, &addr, &addrlen);

    message = (char*) malloc(sizeof(char)*(1+strlen(buffer)));
    strcpy(message, buffer);
    
    token = strtok(message, " \n"); 

    if (token != NULL) {
        result = checkProtocol(token);
    }
    free(message);

    switch (result) {
        case REG:
            buffer = reg(buffer);
            break;

        case LTP:
            buffer = ltp(buffer);
            break;

        case PTP:
            buffer = ptp(buffer);
            break;

        case LQU:
            buffer = lqu(buffer);
            break;

        default:
            free(buffer);
            printf("Unexpected protocol message received, notifying client.\n");
            sendMessageUDP("ERR\n", fd, addr, addrlen);
            return;
    }

    sendMessageUDP(buffer, fd, addr, addrlen);
    free(buffer);
}

int main(int argc, char **argv) {

    int option, p = 0, fdUDP, fdTCP, err;
    char *fsport = PORT, *buffer;
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
    if ((stat("topics", &s) == -1) && mkdir("topics", 0777)) {              
        printf("Server could not create topics folder, exiting now\n");   //substituir por error
        exit(1);
    }
    nTopics = dirSize("topics");

    //Initializing UDP socket
    fdUDP = startUDP(NULL, fsport);

    //Initializing TCP socket
    fdTCP = startTCP(NULL, fsport);
    
    while (1) { 
        /*Mask initialization*/
        FD_ZERO(&rfds);
        FD_SET(0, &rfds);
        FD_SET(fdUDP, &rfds);
        FD_SET(fdTCP, &rfds);

        /*Waits for commands*/
        err = select(max(fdUDP, fdTCP)+1, &rfds, (fd_set*) NULL, (fd_set*) NULL, (struct timeval *)NULL);
        if (err <= 0) { error(2); }

        /*Reads input from an STDIN  --- DEBUG*/  
        if (FD_ISSET(0, &rfds)) {
            char buffer[20] = "";
            while (buffer[0] != '\n') {
                read(0, buffer, 1);  
            };
            break;

        }

        /*Reads input from an UDP connection*/
        if (FD_ISSET(fdUDP, &rfds)) {
            handleMessageUDP(fdUDP);
        }

        /*Reads input from an TCP connection*/
        if (FD_ISSET(fdTCP, &rfds)) {
            handleMessageTCP(fdTCP);
        }
    }

    close(fdUDP);
    close(fdTCP);
    return 0;
}

/*
CHECKAR SE MENSAGENS TERMINAM COM UM \N NO CLIENTE
*/