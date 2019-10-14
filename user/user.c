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
#define STANDARDBUFFERSIZE 256
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

char* readTCP(int fdTCP, size_t nLeft, char* ptr) {
    // reads into 256 byte array, returns pointer to position after last read
    size_t nRead = 0;
    while (nLeft > 0) {
        nRead = read(fdTCP, ptr, nLeft);
        if (nRead == -1) error(2);
        else if (nRead == 0) break;
        nLeft -= nRead;
        ptr += nRead;
    }
    return ptr;
}

char* writeTCP(int fdTCP, size_t nLeft, char* ptr) {
    size_t nWritten = 0;
    while (nLeft > 0) {
        nWritten = write(fdTCP, ptr, nLeft);
        if (nWritten == -1) error(2);
        nLeft -= nWritten;
        ptr += nWritten;
    }
    return ptr;
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

int checkIfNumber(char* token) {
    int size;

    if (token != NULL) {
        size = strlen(token);
        for (int i = 0; i < size; i++) {
            if (token[i] < '0' || token[i] > '9') {
                return 0;
            }
        }
        return 1;
    }
    return 0;
}

int checkIfUserID(char* token) {
    if (token != NULL && strlen(token) == 5) {
        for (int i = 0; i < 5; i++) {
            if (token[i] < '0' || token[i] > '9') {
                return 0;
            }
        }
        return 1;
    }
    return 0;
}

char* registerID(int fdUDP, char* token) {

    int invalid = 0, n = 0;
    char messageSent[MESSAGE_SIZE] = "", messageReceived[MESSAGE_SIZE] = "";

    memset(messageSent, '\0', MESSAGE_SIZE);
    memset(messageReceived, '\0', MESSAGE_SIZE);

    // Verifies validity of userID introduced
    token = strtok(NULL, "\n");
    if (!checkIfUserID(token)) {
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

char* questionGetAux(int isAnswer, int fdTCP, char* messageReceived, char* ptr, char* topic, char* question) {
    char *token, *extension, fileName[STANDARDBUFFERSIZE], filePath[STANDARDBUFFERSIZE], an[2];
    int totalSize, numSize = 1;
    FILE* fp;

    if (isAnswer) {
        ptr = readTCP(fdTCP, 2, ptr);

        *ptr = '\0';
        token = strtok(ptr - 2, " ");

        if (atoi(token) < 0 || atoi(token) > 10) error(2);
        strcpy(an, token);

        ptr = readTCP(fdTCP, 7, ptr);
    }
    else {
        token = strtok(NULL, " ");
    }
    
    ptr = readTCP(fdTCP, 1, ptr);
    while (*(ptr - 1) != ' ') {
        ptr = readTCP(fdTCP, 1, ptr);
        numSize++;
    }

    *ptr = '\0';
    token = strtok((ptr - numSize), " ");

    totalSize = atoi(token);

    strcpy(filePath, topic); strcat(filePath, "/");
    strcat(filePath, question);

    if (isAnswer) {
        strcat(filePath, "_"); strcat(filePath, an);
    }

    strcpy(fileName, filePath);
    fp = fopen(strcat(fileName, ".txt"), "w");

    memset(messageReceived, '\0', STANDARDBUFFERSIZE);
    ptr = messageReceived;

    while (totalSize >= STANDARDBUFFERSIZE) {
        ptr = readTCP(fdTCP, STANDARDBUFFERSIZE - 1, ptr);
        *ptr = '\0';
        fwrite(messageReceived, 1, STANDARDBUFFERSIZE - 1, fp);
        ptr = messageReceived;
        totalSize -= (STANDARDBUFFERSIZE - 1);
    }

    ptr = readTCP(fdTCP, totalSize, ptr);
    *ptr = '\0';
    fwrite(messageReceived, 1, totalSize, fp);

    fclose(fp);

    ptr = messageReceived;
    ptr = readTCP(fdTCP, 2, ptr);
    *ptr = '\0';
    token = strtok(messageReceived, " ");

    if (!strcmp(token, "1")) {
        readTCP(fdTCP, 1, ptr);
        memset(fileName, '\0', STANDARDBUFFERSIZE);
        strcpy(fileName, filePath);

        readTCP(fdTCP, 4, ptr);
        *(ptr + 4) = '\0';
        extension = strtok(ptr, " ");
        strcat(fileName, ".");
        strcat(fileName, extension);

        numSize = 1;
        ptr = readTCP(fdTCP, 1, ptr);
        while (*(ptr - 1) != ' ') {
            ptr = readTCP(fdTCP, 1, ptr);
            numSize++;
        }

        *ptr = '\0';
        token = strtok((ptr - numSize), " ");
        totalSize = atoi(token);

        memset(messageReceived, '\0', STANDARDBUFFERSIZE);
        ptr = messageReceived;

        fp = fopen(fileName, "wb");
        
        while (totalSize > 0) {
            if (totalSize > STANDARDBUFFERSIZE) {
                readTCP(fdTCP, STANDARDBUFFERSIZE, ptr);
                fwrite(ptr, 1, STANDARDBUFFERSIZE, fp);
            }
            else {
                readTCP(fdTCP, totalSize, ptr);
                fwrite(ptr, 1, totalSize, fp);
            }
            totalSize -= STANDARDBUFFERSIZE;
        }

        fclose(fp);

        memset(messageReceived, '\0', STANDARDBUFFERSIZE);
        ptr = messageReceived;
    }

    return ptr;
}

void questionGet(int commandType, int fdTCP, char* token, char* topic, int nQuestions, char** qList) {

    int n, i, questionSelected, numSize, nAnswers;
    ssize_t nBytes, nLeft, nWritten;
    char messageSent[MESSAGE_SIZE] = "", messageReceived[STANDARDBUFFERSIZE] = "", *token2, *ptr = messageSent;
    FILE *fp;

    memset(messageSent, '\0', MESSAGE_SIZE);
    memset(messageReceived, '\0', STANDARDBUFFERSIZE);

    token = strtok(NULL, " \n");
    if ((token == NULL) || (strtok(NULL, "\n") != NULL)) {
        printf("ERR: Format incorrect. Should be: \"question_get question\" with topic being a non-empty string\n");
        return;
    }

    if (!commandType) {
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
    }
    else if (atoi(token) > 0 && atoi(token) <= nQuestions) {
        questionSelected = atoi(token) - 1;
        token2 = strtok(qList[questionSelected], ":");
    }
    else {
        printf("ERR: Invalid question selected\n");
        return;
    }

    strcat(messageSent, "GQU "); strcat(messageSent, topic); strcat(messageSent, " "); strcat(messageSent, token2); strcat(messageSent, "\n");

    n = connect(fdTCP, resTCP->ai_addr, resTCP->ai_addrlen);
    if (n == -1) error(2);

    nBytes = strlen(messageSent);
    ptr = messageSent;

    writeTCP(fdTCP, nBytes, ptr);

    ptr = messageReceived;
    ptr = readTCP(fdTCP, 10, ptr);
    
    if (!strcmp(messageReceived, "QGR EOF\n")) {
        printf("No such query or topic are available\n");
        return;
    }

    if (!strcmp(messageReceived, "QGR ERR\n")) {
        printf("Request formulated incorrectly\n");
        return;
    }
    
    token2 = strtok(messageReceived, " ");

    mkdir(topic, 0777);

    ptr = questionGetAux(0, fdTCP, messageReceived, ptr, topic, qList[questionSelected]);
    
    ptr = readTCP(fdTCP, 2, ptr);
    numSize = 1;

    while (*(ptr - 1) != ' ' && *(ptr - 1) != '\n') {
        ptr = readTCP(fdTCP, 1, ptr);
        numSize++;
    }

    if (*(ptr - 1) == '\n') {
        return;
    }

    *ptr = '\0';
    token = strtok((ptr - numSize), " ");

    nAnswers = atoi(token);
    
    while (nAnswers) {
        ptr = questionGetAux(1, fdTCP, messageReceived, ptr, topic, qList[questionSelected]);
        readTCP(fdTCP, 1, ptr);
        nAnswers -= 1;
    }
}

void question_submit(int fdTCP, char* token, char* topic, char* userID) {
    char messageSent[STANDARDBUFFERSIZE] = "", messageReceived[MESSAGE_SIZE] = "", *ptr = messageSent, *token2, totalSizeStr[10], textFileName[MESSAGE_SIZE];
    FILE *fp;
    struct stat st;
    int n, i, totalSize, nRead;

    memset(messageSent, '\0', MESSAGE_SIZE);
    memset(messageReceived, '\0', STANDARDBUFFERSIZE);

    token2 = strtok(token, " ");
    if (token2 == NULL) {
        printf("Bad command\n");
        return;
    }

    strcat(messageSent, "QUS "); strcat(messageSent, userID); strcat(messageSent, " "); strcat(messageSent, topic); strcat(messageSent, " "); strcat(messageSent, token2);

    token2 = strtok(NULL, " \n");
    if (token2 == NULL) {
        printf("Bad command\n");
        return;
    }

    strcpy(textFileName, token2);
    strcat(textFileName, ".txt");

    n = stat(textFileName, &st);
    if (n) {
        printf("One or more selected files unavailable\n");
        return;
    }
    
    totalSize = st.st_size;
    sprintf(totalSizeStr, "%d", totalSize);

    strcat(messageSent, " "); strcat(messageSent, totalSizeStr); strcat(messageSent, " ");
    ptr = messageSent;

    n = connect(fdTCP, resTCP->ai_addr, resTCP->ai_addrlen);
    if (n == -1) error(2);
    printf("%s\n", ptr);
    
    writeTCP(fdTCP, strlen(messageSent), ptr);

    memset(messageSent, '\0', MESSAGE_SIZE);

    fp = fopen(textFileName, "r");

    while (totalSize >= STANDARDBUFFERSIZE) {
        nRead = fread(messageSent, 1, STANDARDBUFFERSIZE, fp);

        if (nRead < STANDARDBUFFERSIZE) {
            *(ptr + nRead) = '\0';
        }

        printf("\n%s\n", ptr);
        writeTCP(fdTCP, nRead, ptr);
        totalSize -= (nRead);
    }

    ptr += fread(messageSent, 1, totalSize, fp);
    *ptr = '\0';
    ptr = messageSent;
    printf("\n%s\n", ptr);
    writeTCP(fdTCP, totalSize, ptr);

    token2 = strtok(NULL, " \n");
    if (token2 == NULL) {
        writeTCP(fdTCP, 3, " 0\n");
        printf("asd\n");
    }
    else
        printf("\n%s\n", token2);

    ptr = messageReceived;
    readTCP(fdTCP, 8, ptr);

    if (!strcmp(messageReceived, "QUR DUP\n")) {
        printf("Question already exists\n");
        return;
    }

    if (!strcmp(messageReceived, "QUR FUL\n")) {
        printf("Question list is full\n");
        return;
    }
    if (!strcmp(messageReceived, "QUR NOK\n")) {
        printf("Unknown error\n");
        return;
    }
    
    printf("%s\n", messageReceived);

    fclose(fp);
    printf("finish\n");
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
            case QG:
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

                    if (result == QUESTION_GET)
                        questionGet(0, fdTCP, token, tList[sTopic], nQuestions, qList);
                    else
                        questionGet(1, fdTCP, token, tList[sTopic], nQuestions, qList);

                    freeaddrinfo(resTCP);
                    close(fdTCP);
                }

                break;

            case QUESTION_SUBMIT:
                if (strlen(userID) != 5) {
                    printf("There is no user registered\n");
                }
                else if (sTopic == -1) {
                    printf("ERR: Missing information. No topic has been selected\n");
                }
                else {
                    hints.ai_socktype = SOCK_STREAM;

                    s = getaddrinfo(fsip, fsport, &hints, &resTCP);
                    if (s != 0) error(2);

                    fdTCP = socket(resTCP->ai_family, resTCP->ai_socktype, resTCP->ai_protocol);
                    if (fdTCP == -1) error(2);

                    token = strtok(NULL, "\n");

                    question_submit(fdTCP, token, tList[sTopic], userID);

                    freeaddrinfo(resTCP);
                    close(fdTCP);
                }
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