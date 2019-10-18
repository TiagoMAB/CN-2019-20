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
#include <sys/stat.h>
#include <dirent.h>
#include "../util/util.h"

#define PORT "58020"
#define MAX_BUFFER_SIZE 4096
#define STD_BUFFER_SIZE 256
#define MESSAGE_SIZE 30
#define ID_SIZE 6
#define TOPIC_SIZE 17
#define QUESTION_SIZE 20
#define NAME_SIZE 11
#define MAX_PATH_SIZE 57

extern int errno;

enum options {  REGISTER, TOPIC_LIST, TOPIC_PROPOSE, QUESTION_LIST, QUESTION_SUBMIT, ANSWER_SUBMIT,  
                TOPIC_SELECT, TS, QUESTION_GET, QG, EXIT };

socklen_t addrlen;
struct sockaddr_in addr;
struct addrinfo *resUDP;
char pidStr[5], user[ID_SIZE], gQuestion[QUESTION_SIZE+1];

void deleteDirectory() {
    char path[5] = "\0"; strcat(path, pidStr);
    char *fileName1, *fileName2;
    size_t fileNameSize1, fileNameSize2, pathSize;
    DIR *dir1 = opendir(path);
    DIR *dir2;
    struct dirent *p1, *p2;

    pathSize = strlen(path);

    while ((p1 = readdir(dir1)) != NULL) {
        if (!strcmp(p1->d_name, ".") || !strcmp(p1->d_name, ".."))
        {
             continue;
        }

        fileNameSize1 = pathSize + strlen(p1->d_name) + 2;
        fileName1 = malloc(fileNameSize1);

        snprintf(fileName1, fileNameSize1, "%s/%s", path, p1->d_name);
        dir2 = opendir(fileName1);

        while((p2 = readdir(dir2)) != NULL) {
            if (!strcmp(p2->d_name, ".") || !strcmp(p2->d_name, ".."))
            {
                continue;
            }

            fileNameSize2 = fileNameSize1 + strlen(p2->d_name) + 2;
            fileName2 = malloc(fileNameSize2);

            snprintf(fileName2, fileNameSize2, "%s/%s", fileName1, p2->d_name);
            unlink(fileName2);
            free(fileName2);
        }
        
        rmdir(fileName1);
        free(fileName1);
        closedir(dir2);
    }
    
    rmdir(path);
    closedir(dir1);
}

void error(int error) {
    
    switch (error) {
    case 1:
        fprintf(stdout, "ERR: Format incorrect. Should be: ./user [-n FSIP] [-p FSport]\n");
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

void freeList(char** list, int listSize) {
    for (int i = 0; i < listSize; i++) {
        free(list[i]);
    }

    free(list);
}

int command_strcmp(char *token) {

    char *opts[] = {"register", "reg", "topic_list", "tl", "topic_propose", 
                    "tp", "question_list", "ql", "question_submit", "qs", "answer_submit", 
                    "as", "topic_select", "ts", "question_get", "qg", "exit"};
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

char* registerID(int fdUDP, char* token) {

    int invalid = 0, n = 0;
    char request[MESSAGE_SIZE] = "", answer[MESSAGE_SIZE] = "";

    // Verifies validity of userID introduced
    token = strtok(NULL, "\n");
    if (!checkUser(token)) {
        printf("ERR: Format incorrect\n");
        return " ";
    }

    // If userID is valid, sends the information to the server with the format "REG USERID" 
    // and waits a response from it. If the response is "RGR OK", the user is registered
    // If "RGR NOK" is received, it was not possible to register user
    if (!invalid) {
        sprintf(request, "REG %s\n", token);

        n = sendto(fdUDP, request, strlen(request), 0, resUDP->ai_addr, resUDP->ai_addrlen);
        if (n == -1) error(2);
        
        addrlen = sizeof(addr);
        n = recvfrom(fdUDP, answer, MESSAGE_SIZE, 0, (struct sockaddr*) &addr, &addrlen);          //check for cycle
        if (n == -1) error(2);

        if (!strcmp(answer, "RGR OK\n")) {
            printf("The user has been successfully registered\n");
            return token;
        }
        else if (!strcmp(answer, "RGR NOK\n")) {
            printf("ERR: Impossible to register user \"%s\" \n", token); 
        }
        else { 
            printf("ERR: Incorrect protocol message received\n"); 
        }
        return " ";
    }
}

char** topicList(int fdUDP, int *nTopics, char** tList) {

    int n = 0, i = 0;
    char answer[MAX_BUFFER_SIZE] = "", *token = NULL;

    memset(answer, '\0', MAX_BUFFER_SIZE);

    n = sendto(fdUDP, "LTP\n", 4, 0, resUDP->ai_addr, resUDP->ai_addrlen);
    if (n == -1) error(2);
    
    addrlen = sizeof(addr);
    n = recvfrom(fdUDP, answer, MAX_BUFFER_SIZE, 0, (struct sockaddr*) &addr, &addrlen);
    if ((n == -1) || (n == MAX_BUFFER_SIZE - 2)) {
        error(3);                                                //to check
    }
    answer[n] = '\0';

    if (!strcmp(answer, "LTR 0\n")) {
        printf("No topics are yet available\n");
        return tList;
    }

    token = strtok(answer, " ");
    if (strcmp(answer, "LTR")) {
        error(2);
    }

    if (*nTopics != 0) {
        freeList(tList, *nTopics);
    }

    token = strtok(NULL, " ");
    *nTopics = atoi(token);
    if (*nTopics == 0) {
        printf("There are no topics available\n");
        return NULL;
    }

    tList = (char**) malloc(sizeof(char*) * (*nTopics));
    while ((token = strtok(NULL, " \n")) != NULL) {
        tList[i] = (char*) malloc(sizeof(char) * 21);
        strcpy(tList[i++], token);
    }
    
    for (int i = 0; i < *nTopics; i++) {
        token = strtok(tList[i], ":");
        printf("Topic %d: %s by user: ", i + 1, token);
        token = strtok(NULL, ":");
        printf("%s\n", token);
    }

    return tList;
}

int selectTopicT(char* token, char** tList, int nTopics) {

    char* token2;

    token = strtok(NULL, " \n");
    if ((token == NULL) || (strtok(NULL, "\n") != NULL)) {
        printf("ERR: Format incorrect. Should be: \"topic_select topic\" with topic being a non-empty string\n");
        return -1;
    }

    for (int i = 0; i < nTopics; i++) {
        token2 = strtok(tList[i], ":");
        if (!strcmp(token, token2)) {
            printf("Topic selected: %d %s\n", i + 1, token2);
            return i;
        }
    }

    printf("ERR: Invalid topic selected\n");
    return -1;
}

int selectTopicN(char* token, char** tList, int nTopics) {

    char* token2, n;

    token = strtok(NULL, "\n");
    if ((token == NULL) || (strlen(token) > 2)) {
        printf("ERR: Format incorrect. Should be: \"ts topic_number\" with topic_number being a positive (>0) integer within the amount of topics\n");
        return -1;
    }
    for (int i = 0; i < 2; i++) {
        if ((token[i] < '0' || token[i] > '9') && (token[i] != '\0')) {
            printf("ERR: Format incorrect. Should be: \"ts topic_number\" with topic_number being a positive (>0) integer within the amount of topics\n");
            return -1;
        }
    }
    
    n = atoi(token);
    if ((n <= 0) || (n > nTopics)) {
        printf("ERR: Format incorrect. Should be: \"ts topic_number\" with topic_number being a positive (>0) integer within the amount of topics\n");
        return -1;
    }

    printf("Topic selected: %d %s\n", n, strtok(tList[n-1], ":"));
    return n - 1;
}

char** topicPropose(int fdUDP, char* token, int* nTopics, int* sTopic, char** tList) {

    int n = 0;
    char request[MESSAGE_SIZE] = "", answer[MESSAGE_SIZE] = "", topic[TOPIC_SIZE] = "";

    token = strtok(NULL, " \n");
    if ((token == NULL) || (strtok(NULL, "\n") != NULL)) {
        printf("ERR: Format incorrect. Should be: \"topic_propose topic\" or \"tp topic\" with topic being a non-empty string\n");
        return tList;
    }

    sprintf(request, "PTP %s %s\n", user, token);

    n = sendto(fdUDP, request, strlen(request), 0, resUDP->ai_addr, resUDP->ai_addrlen);
    if (n == -1) error(2);
    
    addrlen = sizeof(addr);
    n = recvfrom(fdUDP, answer, MESSAGE_SIZE, 0, (struct sockaddr*) &addr, &addrlen);
    if (n == -1) error(2);

    if (!strcmp(answer, "PTR OK\n")) {
        printf("Topic \"%s\" successfully proposed\n", token);
        sprintf(topic, "%s", token);
        
        *sTopic = *nTopics;
        (*nTopics)++;
        if (tList == NULL) {
            tList = (char**) malloc(sizeof(char*));                             //needs checking
        }
        else {
            tList = (char**) realloc(tList, sizeof(char*) * (*nTopics));
        }

        tList[*nTopics - 1] = (char*) malloc(sizeof(char) * TOPIC_SIZE);
        strcpy(tList[*nTopics - 1], topic);
    }
    else if (!strcmp(answer, "PTR DUP\n")) {
        printf("Topic \"%s\" already exists\n", token);
    }
    else if (!strcmp(answer, "PTR FUL\n")) {
        printf("Topic List is full\n");
    }
    else if (!strcmp(answer, "PTR NOK\n")) {
        printf("Proposal of topic \"%s\" was unnsuccesful\n", token);
    }
    else {
        printf("ERR: Incorrect protocol message received\n"); 
    }

    return tList;
}

char** questionList(int fdUDP, int *nQuestions, char* topic, char** qList) {

    int n = 0, i = 0;
    char request[MESSAGE_SIZE] = "", answer[MAX_BUFFER_SIZE] = "", *token = NULL;

    sprintf(request, "LQU %s\n", topic);

    n = sendto(fdUDP, request, strlen(request), 0, resUDP->ai_addr, resUDP->ai_addrlen);
    if (n == -1) error(2);
    
    addrlen = sizeof(addr);
    n = recvfrom(fdUDP, answer, MAX_BUFFER_SIZE, 0, (struct sockaddr*) &addr, &addrlen);
    if ((n == -1) || (n == MAX_BUFFER_SIZE - 2))  {
        error(3);                                                           //too check
    }
    answer[n] = '\0';

    fprintf(stderr, "%s", answer);                                          // TO REMOVE

    if (!strcmp(answer, "LQR 0\n")) {
        printf("No questions titles are yet available\n");
        return qList;
    }

    token = strtok(answer, " ");
    if (strcmp(answer, "LQR")) {
        error(2);
    } 

    if (*nQuestions != 0) {
        freeList(qList, *nQuestions);
    }

    token = strtok(NULL, " ");
    *nQuestions = atoi(token);
    if (*nQuestions == 0) {
        error(2);
    }

    qList = (char**) malloc(sizeof(char*) * (*nQuestions));

    while ((token = strtok(NULL, " \n")) != NULL) {
        qList[i] = (char*) malloc(sizeof(char) * QUESTION_SIZE);
        strcpy(qList[i++], token);
    }

    for (int i = 0; i < *nQuestions; i++) {
        token = strtok(qList[i], ":");
        printf("Question %d: %s by user: ", i + 1, token);
        token = strtok(NULL, ":");
        printf("%s with ", token);
        token = strtok(NULL, ":");
        printf("%s answers\n", token);
    }
    
    return qList;
}

int checkQuestion(char* token, char** qList, int nQuestions) {

    char* question;
    for (int i = 0; i < nQuestions; i++) {
        question = strtok(qList[i], ":");
        
        if (!strcmp(token, question)) {
            return i;
        }
    }
    return 0;
}

void questionGet(int fd, int flag, int nQuestions, char* topic, char** qList) {

    char *token, *question, *end, request[MESSAGE_SIZE], *id, path[MAX_PATH_SIZE], *N, *AN, answer[MAX_PATH_SIZE];
    char* message;
    long s, index;

    token = strtok(NULL, " \n");
    if ((token == NULL) || (strtok(NULL, "\n") != NULL)) {
        printf("ERR: Format incorrect. Should be: \"topic_select topic\" with topic being a non-empty string\n");
        return;
    }

    if (flag) {
        if (verifyName(token)) {
            printf("ERR: Incorrect format.\n");
            return;
        }
        else if ((index = checkQuestion(token, qList, nQuestions)) == 0) {
            printf("ERR: No such question\n");
            return;
        }
        question = qList[index];
    }
    else {
        index = strtol(token, &end, 10) - 1;
        if (end[0] != '\0' || index < 0 || index > nQuestions) { 
            printf("ERR: Format incorrect or no such question\n");
            return;
        }
        question = qList[index];
    }

    sprintf(request, "GQU %s %s\n", topic, question);
    sendMessageTCP(request, strlen(request), fd);

    message = readToken(id, fd, 1);
    if (message[0] == '\0' || message[strlen(message)-1] == '\n' || strcmp(message, "QGR")) {
        printf("ERR: An unexpected protocol message was received\n");
        free(message);
        return;
    }
    free(message);

    sprintf(path, "%s/%s", pidStr, topic);
    
    if (readAndSave(fd, path, question) != 2) {
        printf("ERR: An unexpected protocol message was received\n");
        return;
    }

    N = readToken(N, fd, 1);
    if (N[0] == '\0') {
        printf("ERR: An unexpected protocol message was received\n");
        free(N);
        return;
    }
    else if (N[strlen(N)-1] == '\n') {
        printf("Question succesfully downloaded\n");
        free(N);
        strcpy(gQuestion, question);
        return;
    }

    s = strtol(N, &end, 10);
    if (end[0] != '\0' || s < 0 || s > 10) { 
        printf("ERR: An unexpected protocol message was received\n");
        free(N);
        return;
    }
    free(N);

    for (int i = 0; i < s; i++) {
        AN = readToken(AN, fd, 1);
        if (AN[0] == '\0' || AN[strlen(AN)-1] == '\n') {
            printf("ERR: An unexpected protocol message was received\n");
            free(AN);
            return;
        }
        sprintf(answer, "%s_%s", question, AN);

        if (readAndSave(fd, path, answer) == 1) {
            printf("ERR: An unexpected protocol message was received\n");
            free(AN);
            return;
        }
        printf("Answer %s succesfully downloaded\n", AN);
        free(AN);
    }

    strcpy(gQuestion, question);
    printf("Return: %ld\n", index);
    return;
} 

int sendInfo(int fd, char* request, char* path1, char* path2) {
    
    sendMessageTCP(request, strlen(request), fd);             //falta check

    readAndSend(path1, "rb", fd);             //falta check
    if (path2 != NULL) {
        char *ext, pathCopy[MAX_PATH_SIZE];
        strcpy(pathCopy, path2);
        strtok(pathCopy, ".");
        ext = strtok(NULL, "");
        sprintf(pathCopy, " 1 %s", ext);
        sendMessageTCP(pathCopy, strlen(pathCopy), fd);
        readAndSend(path2, "rb", fd);
    }
    else {
        sendMessageTCP(" 0", 2, fd);
    }

    sendMessageTCP("\n", 1, fd);
}

char** questionSubmit(int fd, int* sQuestion, char* topic, char** qList) {

    char* question, *token, *pathIMG = NULL, request[MAX_PATH_SIZE];
    char* path = NULL, *pathText, *answer;

    question = strtok(NULL, " \n");
    if (verifyName(question)) {
        printf("ERR: Question name not valid\n");
        return qList;
    }

    path = strtok(NULL, " \n");
    if (path == NULL){
        printf("ERR: Invalid text file\n");
        return qList;
    }

    pathText = (char*) malloc(sizeof(char)*(strlen(path)+5));
    sprintf(pathText, "%s.txt", path);
    sprintf(request, "QUS %s %s %s", user, topic, question);

    pathIMG = strtok(NULL, "\n");
    
    sendInfo(fd, request, pathText, pathIMG);
    
    free(pathText);
    
    answer = readToken(answer, fd, 1);
    if (answer[0] == '\0' || answer[strlen(answer)-1] == '\n' || strcmp(answer, "QUR")) {
        printf("ERR: An unexpected protocol message was received\n");
        free(answer);
        return qList;
    }
    free(answer);
    
    answer = readToken(answer, fd, 1);
    if (answer[0] == '\0') {
        printf("ERR: An unexpected protocol message was received\n");
    }
    else if (!strcmp(answer, "OK\n")) {
        printf("The question named --%s-- has been successfully registered\n", question);
        strcpy(gQuestion, question);
    }
    else if (!strcmp(answer, "DUP\n")) {
        printf("The question named --%s-- was reported as a duplicate\n", question);
    }
    else if (!strcmp(answer, "FUL\n")) {
        printf("There question list for this topic is already full\n");
    }
    else if (!strcmp(answer, "NOK\n")) {
        printf("It was not possible to register this question\n");
    } 
    else {
        printf("ERR: An unexpected protocol message was received\n");
    }

    free(answer);
    return qList;
}

void answerSubmit(int fd, char* topic, char* question) {

    char* token, *pathIMG = NULL, request[MAX_PATH_SIZE];
    char* path = NULL, pathText[MAX_PATH_SIZE] = "", *answer;

    path = strtok(NULL, " \n");
    if (path == NULL){
        printf("ERR: Invalid text file\n");
        return;
    }

    sprintf(pathText, "%s.txt", path);
    sprintf(request, "ANS %s %s %s", user, topic, question);

    pathIMG = strtok(NULL, "\n");
    
    sendInfo(fd, request, pathText, pathIMG);
    
    answer = readToken(answer, fd, 1);
    if (answer[0] == '\0' || answer[strlen(answer)-1] == '\n' || strcmp(answer, "ANR")) {
        printf("ERR: An unexpected protocol message was received\n");
        free(answer);
        return;
    }
    free(answer);
    
    answer = readToken(answer, fd, 1);
    if (answer[0] == '\0') {
        printf("ERR: An unexpected protocol message was received\n");
    }
    else if (!strcmp(answer, "OK\n")) {
        printf("An answer to the question named --%s-- has been successfully registered\n", question);
    }
    else if (!strcmp(answer, "FUL\n")) {
        printf("There question list for this topic is already full\n");
    }
    else if (!strcmp(answer, "NOK\n")) {
        printf("It was not possible to register this question\n");
    } 
    else {
        printf("ERR: An unexpected protocol message was received\n");
    }

    free(answer);
    return;
}


int main(int argc, char **argv) {

    int option, n = 0, p = 0, fdUDP, fdTCP, nTopics = 0, nQuestions = 0, sTopic = -1;
    int sQuestion = -1;
    char *fsip = "localhost", *fsport = PORT, command[MAX_BUFFER_SIZE] = "", *token = NULL;
    int result = -1, pid;
    ssize_t s;
    struct addrinfo hints;
    char **tList = NULL, **qList = NULL;

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

    pid = getpid();
    sprintf(pidStr, "%d", pid);
    printf("%s", pidStr);
    mkdir(pidStr, 0777);

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_PASSIVE|AI_NUMERICSERV;;

    s = getaddrinfo(fsip, fsport, &hints, &resUDP);
    if (s != 0) error(2);

    fdUDP = socket(resUDP->ai_family, resUDP->ai_socktype, resUDP->ai_protocol);
    if (fdUDP == -1) error(2);
    
    memset(command, '\0', MAX_BUFFER_SIZE);

    while (result != EXIT) {
        result = -1;
        printf("Enter command: ");

          // TODO substituir
        fgets(command, MAX_BUFFER_SIZE, stdin);
        token = strtok(command, " \n"); 

        if (token != NULL) {
            result = command_strcmp(token);
            printf("%d", result);
        }

        switch (result) {
            case REGISTER:
                if (strlen(user) != 5) {
                    strcpy(user, registerID(fdUDP, token));
                }
                else {
                    printf("User %s is already registered\n", user);
                }
                break;

            case TOPIC_LIST:
                token = strtok(NULL, "\n");
                if (token != NULL) {
                    printf("ERR: Format incorrect. Should be: \"topic_list\" or \"tl\"\n");
                }
                else {
                    tList = topicList(fdUDP, &nTopics, tList);
                    sTopic = -1;
                }

                break;
            
            case TOPIC_SELECT:
                if ((nTopics) == 0) {
                    printf("ERR: No topics available\n");
                }
                else {
                    sTopic = selectTopicT(token, tList, nTopics);
                }

                if (nQuestions > 0) {
                    freeList(qList, nQuestions);
                    qList = NULL;
                    nQuestions = 0;
                    sQuestion = -1;
                }

                break;
            
            case TS:
                if ((nTopics) == 0) {
                    printf("ERR: No topics available\n");
                }
                else {
                    sTopic = selectTopicN(token, tList, nTopics);
                }

                if (nQuestions > 0) {
                    freeList(qList, nQuestions);
                    qList = NULL;
                    nQuestions = 0;
                    sQuestion = -1;
                }
                
                break;
            
            case TOPIC_PROPOSE:
                if (strlen(user) != 0) {
                    tList = topicPropose(fdUDP, token, &nTopics, &sTopic, tList);
                }
                else {
                    printf("ERR: Missing information. No user has been registered\n");
                }

                break;

            case QUESTION_LIST:
                token = strtok(NULL, "\n");
                if ((token != NULL)) {
                    printf("ERR: Format incorrect. Should be: \"question_list\" or \"ql\"\n");
                }
                else if (sTopic == -1) {
                    printf("ERR: Missing information. No topic has been selected\n");
                }
                else { 
                    qList = questionList(fdUDP, &nQuestions, tList[sTopic], qList);
                    printf("%d\n", nQuestions);
                }

                break;

            case QUESTION_GET:
            case QG:
                if ((sTopic == -1) || (nQuestions == 0)) {
                    printf("ERR: Missing information. No topic or question has been selected\n");
                }
                else {
                    if ((fdTCP = startTCP(fsip, fsport, 0)) == -1) { exit(1); }  

                    if (result == QUESTION_GET)
                        questionGet(fdTCP, 1, nQuestions, tList[sTopic], qList);
                    else
                        questionGet(fdTCP, 0, nQuestions, tList[sTopic], qList);

                    close(fdTCP);
                }

                break;

            case QUESTION_SUBMIT:
                if (strlen(user) != 5) {
                    printf("There is no user registered\n");
                }
                else if (sTopic == -1) {
                    printf("ERR: Missing information. No topic has been selected\n");
                }
                else {
                    if ((fdTCP = startTCP(fsip, fsport, 0)) == -1) { exit(1); }  

                    qList = questionSubmit(fdTCP, &sQuestion, tList[sTopic], qList);

                    close(fdTCP);
                }

                break;

            case ANSWER_SUBMIT:
                if (strlen(user) != 5) {
                    printf("There is no user registered\n");
                }
                else if ((sTopic == -1) || (nQuestions <= 0)) {
                    printf("ERR: Missing information. No topic or question has been selected\n");
                }
                else {
                    if ((fdTCP = startTCP(fsip, fsport, 0)) == -1) { exit(1); }      //check is needs a message to show to user

                    printf("|%s|\n", gQuestion);
                    answerSubmit(fdTCP, tList[sTopic], gQuestion);
                    close(fdTCP);
                }

                break;

            case EXIT:
                break;
            default:
                fprintf(stdout, "Command does not exist\n");
                break;
        }

        memset(command, 0, sizeof(command));
    }

    if (tList != NULL)
        freeList(tList, nTopics);
    if (qList != NULL)
        freeList(qList, nQuestions);
    
    deleteDirectory();

    freeaddrinfo(resUDP);
    close(fdUDP);

    return 0;
}