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

void error(int error) {         //check if will use

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

int checkProtocol(char* token) {  //check

    char *opts[] = { "REG", "LTP", "PTP", "LQU", "GQU", "QUS", "ANS" };
    int i;

    for (i = 0; i < 7; i++) {
        if (!strcmp(token, opts[i])) {
            return i;
        }
    }
    return i;
}

char* getDirContent(char* path, char* answer, int flag) {       //check

    DIR *d;
    FILE *f;
    struct dirent *dir;

    d = opendir(path);
    if (d) {
        while ((dir = readdir(d)) != NULL) {
            if ((dir->d_type == DT_DIR) && strcmp(dir->d_name, ".") && strcmp(dir->d_name, "..")) {
                char subPath[MAX_PATH_SIZE], subDir[11], id[6];

                strcpy(subDir, dir->d_name);
                sprintf(subPath, "%s/%s/%s_UID.txt", path, subDir, subDir);

                if ((f = fopen(subPath, "r")) == NULL ) { free(answer); return NULL; }
                sprintf(subPath, "%s/%s", path, subDir);

                if (fgets(id, 6, f) == NULL ) { free(answer); return NULL; }
                if (flag) {
                    sprintf(answer, "%s %s:%s:%d", answer, subDir, id, dirSize(subPath));
                }
                else {
                    sprintf(answer, "%s %s:%s", answer, subDir, id);
                } 

                if (fclose(f) == EOF) { free(answer); return NULL; }                        //CHAMADA SISTEMA?
            }
        }
        if (closedir(d)) { free(answer); return NULL; }                                //CHAMADA SISTEMA?
        return answer;
    }
    else { free(answer); return NULL; }
}

char* reg(char* buffer) {  //check

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

char* ltp(char* buffer) { //check

    char* answer = (char*)malloc(sizeof(char)*((TOPIC_SIZE)*(MAX_TOPICS+1)));
    strcpy(answer, "ERR\n");

    if (strlen(buffer) == 4) {
        sprintf(answer, "LTR %d", dirSize("topics"));
        answer = getDirContent("topics", answer, 0);
        if (answer == NULL) { 
            free(buffer); 
            answer = (char*)malloc(sizeof(char)*4);
            strcpy(answer, "ERR\n");
            return answer; 
        }
        strcat(answer, "\n");
    }

    free(buffer);
    return answer;
}

char* ptp(char* buffer) {           //check

    char *id, *topic, path[MAX_PATH_SIZE] = "";
    char* answer = (char*)malloc(sizeof(char)*9);
    FILE *f;
    
    strcpy(answer, "PTR NOK\n");
    strtok(buffer, " ");
    id = strtok(NULL, " ");
    topic = strtok(NULL, "\n");

    if (dirSize("topics") == MAX_TOPICS) {
        free(buffer);
        strcpy(answer, "PTR FUL\n");
        return answer;       
    }

    if (!checkUser(id) || verifyName(topic)) {
        free(buffer);
        return answer;
    }

    sprintf(path, "topics/%s", topic);
    if (checkDir(path)) {
        strcpy(answer, "PTR DUP\n");
        free(buffer);
        return answer;
    }
    
    if (mkdir(path, 0777)) { 
        strcpy(answer, "PTR NOK\n");
        return answer; 
    }                         

    sprintf(path, "topics/%s/%s_UID.txt", topic, topic);
    if ((f = fopen(path, "w")) == NULL ) { 
        strcpy(answer, "PTR NOK\n");
        return answer; 
    } 
                  
    fwrite(id, 1, strlen(id), f);
    if (fclose(f) == EOF) {
        strcpy(answer, "PTR NOK\n");
        return answer; 
    }                             
    
    strcpy(answer, "PTR OK\n");

    free(buffer);
    return answer;
}

char* lqu(char *buffer) {       //check

    char* topic, path[MAX_PATH_SIZE] = "", *answer = (char*) malloc(sizeof(char)*(MAX_TOPICS*(TOPIC_SIZE+3)+8));
    int size;

    strcpy(answer, "LQR");
    strtok(buffer, " ");
    topic = strtok(NULL, "\n");

    sprintf(path, "topics/%s", topic);
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
    if (answer == NULL) { 
        free(buffer); 
        answer = (char*)malloc(sizeof(char)*4);
        strcpy(answer, "ERR\n");
        return answer; 
    }
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
    FILE *f;
    int size, qIMG = 0, ansSize = 270, nBytes = 0;
    long fileSize = 0;
    char pathUID[MAX_PATH_SIZE], pathTitle[MAX_PATH_SIZE], pathIMG[MAX_PATH_SIZE], buffer[256] = "", *answer = (char*) malloc(sizeof(char)*ansSize), *buffer2;

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
























int saveFolder(int fd, char* user, char* name, char* path, struct sockaddr_in *addr, socklen_t *addrlen) {  //checked

    char pathUID[MAX_PATH_SIZE] = "", pathTitle[MAX_PATH_SIZE] = "", pathIMG[MAX_PATH_SIZE] = "";
    char *fileSize, *flag, *ext, *imgSize;
    int size;
    FILE *f;

    if (mkdir(path, 0777)) { return 1; }         

    sprintf(pathUID, "%s/%s_UID.txt", path, name);

    if ((f = fopen(pathUID, "w")) == NULL ) { return 1; }
    fwrite(user, 1, strlen(user), f);                   //check
    if (fclose(f) == EOF) { return 1; };      

    sprintf(pathTitle, "%s/%s.txt", path, name);
    if (readAndWrite(pathTitle, "w", 0, fd)) { return 1; }

    readToken(NULL, fd, 0);
    flag = readToken(flag, fd, 1);

    if (!strcmp(flag, "1")) {
        ext = readToken(ext, fd, 0);
        sprintf(pathIMG, "%s/%s.%s", path, name, ext);
        if (readAndWrite(pathIMG, "wb", 0, fd)) { return 1; }
    }
    else if (!strcmp(flag, "0\n")) { return 0; }
    else { return 1; }

    flag = readToken(flag, fd, 1);
    if (!strcmp(flag, "\n")){
        free(flag);
        return 0;
    }

    return 1;
}

void gqu(int fd, struct sockaddr_in *addr, socklen_t *addrlen) {

    char pathTopic[MAX_PATH_SIZE], pathQuestion[MAX_PATH_SIZE], *topic, *question;

    topic = readToken(topic, fd, 1);
    if (topic[strlen(topic)-1] == '\n') {
        sendMessageTCP("QGR NOK\n", 8, fd);
        return;
    }

    question = readToken(question, fd, 1);
    if (question[strlen(question)-1] != '\n') {
        sendMessageTCP("QGR NOK\n", 8, fd);
        return;
    }
    else { question[strlen(question)-1] = '\0'; }

    printf("STDERR: GQU: %s | %s \n", topic, question);

    if (verifyName(topic) || verifyName(question)) {
        sendMessageTCP("QGR ERR\n", 8, fd);
        return;
    }

    sprintf(pathTopic, "topics/%s", topic);
    sprintf(pathQuestion, "topics/%s/%s", topic, question);

    printf("STDERR: Path: %s | Question: %s\n", pathTopic, pathQuestion);
    if (!checkDir(pathTopic) || !checkDir(pathQuestion)) {
        sendMessageTCP("QGR EOF\n", 8, fd);
        return;
    }

    dirInfo(pathQuestion, question, fd, addr, addrlen);
}








void qus(int fd, struct sockaddr_in *addr, socklen_t *addrlen) {            //checked

    char pathTopic[MAX_PATH_SIZE], pathQuestion[MAX_PATH_SIZE], *topic, *question, *user;
        
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

    sprintf(pathTopic, "topics/%s", topic);
    sprintf(pathQuestion, "topics/%s/%s", topic, question);

    if (!checkUser(user) || verifyName(topic) || verifyName(question)) {
        sendMessageTCP("QUR NOK\n", 8, fd);
    }
    else if (checkDir(pathQuestion)) {
        sendMessageTCP("QUR DUP\n", 8, fd);
    }
    else if (dirSize(pathTopic) == MAX_TOPICS) {
        sendMessageTCP("QUR FUL\n", 8, fd);
    }
    else if (saveFolder(fd, user, question, pathQuestion, addr, addrlen)) {
        sendMessageTCP("QUR NOK\n", 8, fd);
    }
    else {
        printf("The user: %s has submitted a question named: %s\n", user, question);
        sendMessageTCP("QUR OK\n", 8, fd);
    }
}


void ans(int fd, struct sockaddr_in *addr, socklen_t *addrlen) {

    char pathQuestion[MAX_PATH_SIZE], pathAnswer[MAX_PATH_SIZE], *topic, *question, *user;
    int size = 0;

    user = readToken(user, fd, 1);
    if (user[strlen(user)-1] == '\n') {
        sendMessageTCP("ANR NOK\n", 8, fd);
        return;
    }

    topic = readToken(topic, fd, 1);
    if (topic[strlen(topic)-1] == '\n') {
        sendMessageTCP("ANR NOK\n", 8, fd);
        return;
    }

    question = readToken(question, fd, 1);
    if (question[strlen(question)-1] == '\n') {
        sendMessageTCP("ANR NOK\n", 8, fd);
        return;
    }

    if (!checkUser(user) || verifyName(topic) || verifyName(question)) {
        sendMessageTCP("ANR NOK\n", 8, fd);
        return;
    }

    sprintf(pathQuestion, "topics/%s/%s", topic, question);
    size = dirSize(pathQuestion);
    sprintf(pathAnswer, "topics/%s/%s/%s_%02d", topic, question, question, size + 1);

    if (size == MAX_TOPICS) {
        sendMessageTCP("ANR FUL\n", 8, fd);
    }
    else if (saveFolder(fd, user, question, pathAnswer, addr, addrlen)) {
        sendMessageTCP("ANR NOK\n", 8, fd);
    }
    else {
        printf("The user: %s has submitted an answer to the question named: %s\n", user, question);
        sendMessageTCP("ANR OK\n", 8, fd);
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