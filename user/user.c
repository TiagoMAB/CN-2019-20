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
int connectedTCP = 0;

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

    memset(messageSent, '\0', MESSAGE_SIZE);
    memset(messageReceived, '\0', MESSAGE_SIZE);

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
        n = recvfrom(fdUDP, messageReceived, MESSAGE_SIZE, 0, (struct sockaddr*) &addr, &addrlen);
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
    char messageReceived[MAXBUFFERSIZE] = "", *token = NULL;

    memset(messageReceived, '\0', MAXBUFFERSIZE);

    n = sendto(fdUDP, "LTP\n", 4, 0, resUDP->ai_addr, resUDP->ai_addrlen);
    if (n == -1) error(2);
    
    addrlen = sizeof(addr);
    n = recvfrom(fdUDP, messageReceived, MAXBUFFERSIZE, 0, (struct sockaddr*) &addr, &addrlen);
    if (n == -1) error(2);
    if (n == MAXBUFFERSIZE - 2) error(3); //message too long, can't be fully recovered
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
        freeList(tList, *nTopics);
    }

    token = strtok(NULL, " ");
    *nTopics = atoi(token);
    if (*nTopics == 0) {
        error(2);
    }

    tList = (char**) malloc(sizeof(char*) * (*nTopics));

    while ((token = strtok(NULL, " \n")) != NULL) {
        tList[i] = (char*) malloc(sizeof(char) * 21);
        strcpy(tList[i++], token); 
    }
    
    //falta verificar se recebemos a resposta toda certa
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
    memset(messageReceived, '\0', MAXBUFFERSIZE);

    strcat(messageSent, "LQU "); strcat(messageSent, topic); strcat(messageSent, "\n");
    fprintf(stderr, "%s", messageSent); // TO REMOVE

    n = sendto(fdUDP, messageSent, strlen(messageSent), 0, resUDP->ai_addr, resUDP->ai_addrlen);
    if (n == -1) error(2);
    
    addrlen = sizeof(addr);
    n = recvfrom(fdUDP, messageReceived, MAXBUFFERSIZE, 0, (struct sockaddr*) &addr, &addrlen);
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
        freeList(qList, *nQuestions);
    }

    token = strtok(NULL, " ");
    *nQuestions = atoi(token);
    if (*nQuestions == 0) {
        error(2);
    }

    qList = (char**) malloc(sizeof(char*) * (*nQuestions));

    while ((token = strtok(NULL, " \n")) != NULL) {
        qList[i] = (char*) malloc(sizeof(char) * 21);
        strcpy(qList[i++], token);
    }
    
    //falta verificar se recebemos a resposta toda certa
    return qList;
}

void questionGet(int fdTCP, char* token, char* topic, int nQuestions, char** qList) {

    int n = 0, i, questionSelected, questionSize;
    ssize_t nBytes, nLeft, nWritten, nRead;
    char messageSent[MESSAGE_SIZE] = "", messageReceived[MAXBUFFERSIZE] = "", *token2, *ptr = messageSent, folderPath[MESSAGE_SIZE] = "", answerFile[MESSAGE_SIZE];
    FILE *fp;

    memset(messageSent, '\0', MESSAGE_SIZE);
    memset(messageReceived, '\0', MAXBUFFERSIZE);

    token = strtok(NULL, " \n");
    if ((token == NULL) || (strtok(NULL, "\n") != NULL)) {
        printf("ERR: Format incorrect. Should be: \"question_get question\" with topic being a non-empty string\n");
        return;
    }

    for (i = 0; i < nQuestions; i++) {
        token2 = strtok(qList[i], ":");
        if (!strcmp(token, token2)) {
            printf("Question selected: %d %s\n", i + 1, token2);
            questionSelected = i;
            break;
        }
    }

    if (i == nQuestions) {
        printf("ERR: Invalid question selected\n");
        return;
    }

    strcat(messageSent, "GQU "); strcat(messageSent, topic); strcat(messageSent, " "); strcat(messageSent, token); strcat(messageSent, "\n");

    n = connect(fdTCP, resTCP->ai_addr, resTCP->ai_addrlen);
    if (n == -1) error(2);

    nBytes = strlen(messageSent);
    nLeft = nBytes;
    while (nLeft > 0) {
        nWritten = write(fdTCP, messageSent, nLeft);
        if (nWritten <= 0) error(2);
        nLeft -= nWritten;
        ptr += nWritten;
    }

    i = 0;
    ptr = messageReceived;
    while (i < MAXBUFFERSIZE && read(fdTCP, ptr, 1) != -1) {        
        i++; ptr++;
    }

    printf("%s", messageReceived);

    if (!strcmp(messageReceived, "QGR EOF\n")) {
        printf("No such query or topic are available\n");
        return;
    }

    if (!strcmp(messageReceived, "QGR ERR\n")) {
        printf("Request formulated incorrectly\n");
        return;
    }

    strtok(messageReceived, " "); strtok(NULL, " ");
    questionSize = atoi(strtok(NULL, " "));
    token2 = strtok(NULL, "\n");

    mkdir(topic, 0777);

    memset(folderPath, '\0', sizeof(folderPath));
    strcpy(folderPath, topic); strcat(folderPath, "/");

    fp = fopen(strcat(strcat(folderPath, qList[questionSelected]), ".txt"), "w");
    printf("%i\n", sizeof(token2));
    fwrite(token2, 1, strlen(token2) , fp);
    fclose(fp);

    if (strtok(NULL, " ") == "1") {
        ; // codigo das imagens
    }

    n = atoi(strtok(NULL, " "));
    /*
    for (i = 0; i < n; i++) {
        memset(answerFile, '\0', sizeof(answerFile));
        strcat(answerFile, folderPath);

        token2 = strtok(NULL, "");

        fp = fopen(); // codigo das respostas
    }*/
}

int main(int argc, char **argv) {

    int option, n = 0, p = 0, fdUDP, fdTCP, nTopics = 0, nQuestions = 0, sTopic = -1;
    char *fsip = "localhost", *fsport = PORT, command[MAXBUFFERSIZE] = "", *token = NULL, userID[MAXUSERIDSIZE];
    int result = -1;
    ssize_t s;
    struct addrinfo hints;
    char **tList, **qList;

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
    
    memset(userID, '\0', MAXUSERIDSIZE);
    memset(command, '\0', MAXBUFFERSIZE);

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

                if (nQuestions > 0) {
                    freeList(qList, nQuestions);
                    nQuestions = 0;
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
                    nQuestions = 0;
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
                if ((token != NULL)) {
                    printf("ERR: Format incorrect. Should be: \"question_list\" or \"ql\"\n");
                }
                else if(sTopic == -1) {
                    printf("ERR: Missing information. No topic has been selected\n");
                }
                else { 
                    qList = questionList(fdUDP, &nQuestions, tList[sTopic], qList);
                }

                break;

            case QUESTION_GET:
                if (sTopic == -1) {
                    printf("ERR: Missing information. No topic has been selected\n");
                }
                else if (nQuestions == 0) {
                    printf("ERR: Execute the question_list instruction first\n");
                }
                else {
                    hints.ai_socktype = SOCK_STREAM;

                    s = getaddrinfo(fsip, fsport, &hints, &resTCP);
                    if (s != 0) error(2);

                    fdTCP = socket(resTCP->ai_family, resTCP->ai_socktype, resTCP->ai_protocol);
                    if (fdTCP == -1) error(2);
                    questionGet(fdTCP, token, tList[sTopic], nQuestions, qList);
                    freeaddrinfo(resTCP);
                    close(fdTCP);
                }

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

        memset(command, 0, sizeof(command));
    }

    freeList(tList, nTopics);
    freeList(qList, nQuestions);

    freeaddrinfo(resUDP);
    close(fdUDP);

    return 0;
}