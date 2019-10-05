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
#define MAXUSERIDSIZE 5 + 1

extern int errno;

enum options {  REGISTER, TOPIC_LIST, TOPIC_PROPOSE, QUESTION_LIST, QUESTION_SUBMIT, QUESTION_ANSWER, 
                TOPIC_SELECT, TS, QUESTION_GET, QG, EXIT };

socklen_t addrlen;
struct sockaddr_in addr;
struct addrinfo *resUDP, *resTCP;

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

char* registerID(int fdUDP, char* token) {

    int invalid = 0, n = 0;
    char messageSent[MESSAGE_SIZE] = "", messageReceived[MESSAGE_SIZE] = "";

    memset(messageSent, 0, MESSAGE_SIZE);
    memset(messageReceived, 0, MESSAGE_SIZE);

    // Verifies validity of userID introduced
    token = strtok(NULL, "\n");
    if (token != NULL && strlen(token) == 5) {
        for (int i = 0; i < 5; i++) {
            if (token[i] < '0' || token[i] > '9') {
                invalid = 1;
                printf("ERR: Format incorrect. Should be: \"register userID\" or \"reg userID\" with userID between 00000 and 99999\n");
                return "";
            }
        }
    }
    else {
        invalid = 1;
        printf("ERR: Format incorrect. Should be: \"register userID\" or \"reg userID\" with userID between 00000 and 99999\n");
        return "";
    }

    // If userID is valid, sends the information to the server with the format "REG USERID" 
    // and waits a response from it. If the response is "RGR OK", the user is registered
    // If "RGR NOK" is received, it was not possible to register user
    if (!invalid) {
        strcat(messageSent, "REG "); strcat(messageSent, token); strcat(messageSent, "\n");
        fprintf(stderr, "STDERR: %s", messageSent); // TO REMOVE

        n = sendto(fdUDP, messageSent, strlen(messageSent), 0, resUDP->ai_addr, resUDP->ai_addrlen);
        if (n == -1) error(2);
        
        addrlen = sizeof(addr);
        n = recvfrom(fdUDP, messageReceived, MESSAGE_SIZE - 1, 0, (struct sockaddr*) &addr, &addrlen);
        if (n == -1) error(2);

        fprintf(stderr, "STDERR: %s", messageReceived); // TO REMOVE

        if (!strcmp(messageReceived, "RGR OK\n")) {
            printf("User \"%s\" registered\n", token);
            return token;
        }
        else if (!strcmp(messageReceived, "RGR NOK\n")) {
            printf("ERR: Impossible to register user \"%s\" \n", token); 
            return 0;
        }
        else { 
            /*terminar graciosamente - ALTER*/
            error(1);
        }
    }
}

char** topicList(int fdUDP, int *nTopics, char** tList) {

    int n = 0, i = 0;
    char messageSent[MESSAGE_SIZE] = "", messageReceived[MAXBUFFERSIZE] = "", *token = NULL;

    memset(messageSent, '\0', MESSAGE_SIZE);

    strcat(messageSent, "LTP"); strcat(messageSent, "\n");
    fprintf(stderr, "%s", messageSent); // TO REMOVE

    n = sendto(fdUDP, messageSent, strlen(messageSent), 0, resUDP->ai_addr, resUDP->ai_addrlen);
    if (n == -1) error(2);
    
    addrlen = sizeof(addr);
    n = recvfrom(fdUDP, messageReceived, MAXBUFFERSIZE - 2, 0, (struct sockaddr*) &addr, &addrlen);
    if (n == -1) error(2);
    if (n == MAXBUFFERSIZE - 2) {
        error(3); //message too long, can't be fully recovered
    }
    messageReceived[n] = '\0';

    fprintf(stderr, "%s", messageReceived); // TO REMOVE

    if (!strcmp(messageReceived, "LTR 0\n")) {
        printf("No topics are yet available\n");
        return tList;
    }

    token = strtok(messageReceived, " ");
    if (strcmp(messageReceived, "LTR")) {
        error(2);
    } 

    if (*nTopics != 0) {
        for (int i = 0; i < *nTopics; i++) {
            free(tList[i]); 
        }
        free(tList);
    }

    token = strtok(NULL, " ");
    *nTopics = atoi(token);
    if (*nTopics == 0) {
        error(2);
    }

    tList = (char**)malloc(sizeof(char*)*(atoi(token)));

    while ((token = strtok(NULL, " \n")) != NULL) {
        tList[i] = (char*)malloc(sizeof(char)*21);
        strcpy(tList[i++], token); 
    }
    
    //falta verificar se recebemos a resposta toda certa
    return tList;
}

int selectTopicT(char* token, char** topicList, int nTopics) {

    char* token2;

    token = strtok(NULL, " \n");
    if ((token == NULL) || (strtok(NULL, "\n") != NULL)) {
        printf("ERR: Format incorrect. Should be: \"topic_select topic\" with topic being a non-empty string\n");
        return -1;
    }

    for (int i = 0; i < nTopics; i++) {
        token2 = strtok(topicList[i], ":");
        if (!strcmp(token, token2)) {
            printf("Topic selected: %d %s\n", i + 1, token2);
            return i;
        }
    }

    printf("ERR: Invalid topic selected\n");
    return -1;
}

int selectTopicN(char* token, char** topicList, int nTopics) {

    char* token2, n;

    token = strtok(NULL, " \n");
    if ((token == NULL) || (strtok(NULL, "\n") != NULL) || (strlen(token) > 2)) {
        printf("ERR: Format incorrect. Should be: \"ts topic_number\" with topic_number being a positive (>0) integer in the XX format within the amount of topics\n");
        return -1;
    }
    for (int i = 0; i < 2; i++) {
        if ((token[i] < '0' || token[i] > '9') && (token[i] != '\0')) {
            printf("ERR: Format incorrect. Should be: \"ts topic_number\" with topic_number being a positive (>0) integer in the XX format within the amount of topics\n");
            return -1;
        }
    }
    
    n = atoi(token);
    if ((n <= 0) || (n > nTopics)) {
        printf("ERR: Format incorrect. Should be: \"ts topic_number\" with topic_number being a positive (>0) integer in the XX format within the amount of topics\n");
        return -1;
    }

    printf("Topic selected: %d %s\n", n, strtok(topicList[n-1], ":"));
    return n - 1;
}

void topicPropose(int fdUDP, char* token, char* userID) {

    int n = 0;
    char messageSent[MESSAGE_SIZE] = "", messageReceived[MESSAGE_SIZE] = "";

    memset(messageSent, '\0', MESSAGE_SIZE);
    memset(messageReceived, '\0', MESSAGE_SIZE);

    token = strtok(NULL, " \n");
    if ((token == NULL) || (strtok(NULL, "\n") != NULL)) {
        printf("ERR: Format incorrect. Should be: \"topic_propose topic\" or \"tp topic\" with topic being a non-empty string\n");
        return;
    }

    strcat(messageSent, "PTP "); strcat(messageSent, userID); strcat(messageSent, " "); strcat(messageSent, token); strcat(messageSent, "\n");

    n = sendto(fdUDP, messageSent, strlen(messageSent), 0, resUDP->ai_addr, resUDP->ai_addrlen);
    if (n == -1) error(2);
    
    addrlen = sizeof(addr);
    n = recvfrom(fdUDP, messageReceived, MESSAGE_SIZE, 0, (struct sockaddr*) &addr, &addrlen);
    if (n == -1) error(2);

    if (!strcmp(messageReceived, "PTR OK\n")) {
        printf("Topic \"%s\" successfully proposed\n", token);
    }
    else if (!strcmp(messageReceived, "PTR DUP\n")) {
        printf("Topic \"%s\" already exists\n", token);
    }
    else if (!strcmp(messageReceived, "PTR FUL\n")) {
        printf("Topic List is full\n");
    }
    else if (!strcmp(messageReceived, "PTR NOK\n")) {
        printf("Proposal of topic \"%s\" was unnsuccesful\n", token);
    }
    else {
        error(2); //maybe alterar para outra coisa que nao sai do programa
    }
}

char** questionList(int fdUDP, int *nQuestions, char* topic, char** qList) {
    int n = 0, i = 0;
    char messageSent[MESSAGE_SIZE] = "", messageReceived[MAXBUFFERSIZE] = "", *token = NULL;

    memset(messageSent, '\0', MESSAGE_SIZE);

    strcat(messageSent, "LQU "); strcat(messageSent, topic); strcat(messageSent, "\n");
    fprintf(stderr, "%s", messageSent); // TO REMOVE

    n = sendto(fdUDP, messageSent, strlen(messageSent), 0, resUDP->ai_addr, resUDP->ai_addrlen);
    if (n == -1) error(2);
    
    addrlen = sizeof(addr);
    n = recvfrom(fdUDP, messageReceived, MAXBUFFERSIZE - 2, 0, (struct sockaddr*) &addr, &addrlen);
    if (n == -1) error(2);
    if (n == MAXBUFFERSIZE - 2) {
        error(3); //message too long, can't be fully recovered
    }
    messageReceived[n] = '\0';

    fprintf(stderr, "%s", messageReceived); // TO REMOVE

    if (!strcmp(messageReceived, "LQR 0\n")) {
        printf("No questions titles are yet available\n");
        return qList;
    }

    token = strtok(messageReceived, " ");
    if (strcmp(messageReceived, "LQR")) {
        error(2);
    } 

    if (*nQuestions != 0) {
        for (int i = 0; i < *nQuestions; i++) {
            free(qList[i]); 
        }
        free(qList);
    }

    token = strtok(NULL, " ");
    *nQuestions = atoi(token);
    if (*nQuestions == 0) {
        error(2);
    }

    qList = (char**)malloc(sizeof(char*)*(atoi(token)));

    while ((token = strtok(NULL, " \n")) != NULL) {
        qList[i] = (char*)malloc(sizeof(char)*21);
        strcpy(qList[i++], token); 
    }
    
    //falta verificar se recebemos a resposta toda certa
    return qList;
}

int main(int argc, char **argv) {

    int option, n = 0, p = 0, fdUDP, fdTCP, nTopics = 0, nQuestions = 0, sTopic = -1;
    char *fsip = "localhost", *fsport = PORT, command[MAXBUFFERSIZE] = "", *token = NULL,
        buffer[128], messageSent[MESSAGE_SIZE], messageReceived[MESSAGE_SIZE], userID[MESSAGE_SIZE];
    int result = -1;
    ssize_t s;
    struct addrinfo hints;
    char **tList;
    char **qList;
    memset(messageSent, '\0', MESSAGE_SIZE);
    memset(messageReceived, '\0', MESSAGE_SIZE);
    memset(userID, '\0', MESSAGE_SIZE);

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
    
    fprintf(stderr, "STDERR: %s %s %i\n", fsip, fsport, optind); // TO REMOVE

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_NUMERICSERV;

    s = getaddrinfo(fsip, fsport, &hints, &resUDP);
    if (s != 0) error(2);

    fdUDP = socket(resUDP->ai_family, resUDP->ai_socktype, resUDP->ai_protocol);
    if (fdUDP == -1) error(2);

    hints.ai_socktype = SOCK_STREAM;

    s = getaddrinfo(fsip, fsport, &hints, &resTCP);
    if (s != 0) error(2);

    fdTCP = socket(resTCP->ai_family, resTCP->ai_socktype, resTCP->ai_protocol);
    if (fdTCP == -1) error(2);

    while (result != EXIT) {
        result = -1;
        printf("Write command: ");

          // TODO substituir
        fgets(command, MAXBUFFERSIZE, stdin);
        token = strtok(command, " \n"); 

        if (token != NULL) {
            result = command_strcmp(token);
        }

        switch (result) {
            case REGISTER:
                if (strlen(userID) != 5) {
                    strcpy(userID, registerID(fdUDP, token));
                }
                else {
                    printf("ERR: This client already has a user registered: %s\n", userID);
                }
                break;

            case TOPIC_LIST:
                token = strtok(NULL, "\n");
                if (token != NULL) {
                    printf("ERR: Format incorrect. Should be: \"topic_list\" or \"tl\"\n");
                }
                else { 
                    tList = topicList(fdUDP, &nTopics, tList);
                }
                break;
            
            case TOPIC_SELECT:
                if ((nTopics) == 0) {
                    printf("ERR: No topics available\n");
                }
                else {
                    sTopic = selectTopicT(token, tList, nTopics);
                }
                break;
            
            case TS:
                if ((nTopics) == 0) {
                    printf("ERR: No topics available\n");
                }
                else {
                    sTopic = selectTopicN(token, tList, nTopics);
                }
                break;
            
            case TOPIC_PROPOSE:
                if (strlen(userID) != 0) {
                    topicPropose(fdUDP, token, userID);
                }
                else {
                    printf("ERR: Missing information. No user has been registered\n");
                }
                break;

            case QUESTION_LIST:
                token = strtok(NULL, "\n");
                if (token != NULL) {
                    printf("ERR: Format incorrect. Should be: \"question_list\" or \"ql\"\n");
                }
                else { 
                    qList = questionList(fdUDP, &nQuestions, tList[sTopic], qList);
                }
                break;
            case QUESTION_GET:
                break;
            case QG:
                break;
            case QUESTION_SUBMIT:
                break;
            case QUESTION_ANSWER:
                break;
            case EXIT:
                break;
            default:
                fprintf(stdout, "Command does not exist\n");
                break;
        }

        memset(messageSent, '\0', MESSAGE_SIZE);
        memset(messageReceived, '\0', MESSAGE_SIZE);
    }
    for (int i = 0; i < nTopics; i++) {
        free(tList[i]); 
    }
    for (int i = 0; i < nQuestions; i++) {
        free(qList[i]); 
    }

    free(tList);
    free(qList);
    freeaddrinfo(resUDP);
    freeaddrinfo(resTCP);
    close(fdUDP);
    close(fdTCP);

    return 0;
}