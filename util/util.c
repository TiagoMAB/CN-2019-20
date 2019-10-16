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
#include "util.h"

#define MAX_PATH_SIZE 57
#define BUFFER_SIZE 2048

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

int verifyName(char* topic) {

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

char* readToken(char* answer, int fdTCP, int flag) {

    int readBytes = 0, size = 2;
    char buffer[2] = "";
    answer = (char*) malloc(sizeof(char)*size);
    memset(answer, '\0', size);

    while (readBytes = read(fdTCP, buffer, 1) && buffer[0] != ' ' && (buffer[0] != '\n' || flag)) {
        buffer[1] = '\0';
        size++;
        answer = (char*) realloc(answer, sizeof(char)*size);
        strcat(answer, buffer);
        if (flag && buffer[0] == '\n') { return answer; }
    }

    return answer;
}

int readAndWrite(char *path, char* mode, int nBytes, int fd) {

    int readBytes = 0;
    char* buffer = (char*) malloc(sizeof(char)*(BUFFER_SIZE+1));
    char* size;
    FILE *f;

    f = fopen(path, mode);

    if (nBytes == 0) {
        size = readToken(size, fd, 0);
        if (!(nBytes = atoi(size))) { return 1; }
    }

    printf("STDERR: ReadWrite Path: %s | Mode: %s | size: %d\n", path, mode, nBytes);

    while (nBytes > BUFFER_SIZE) {
        readBytes = read(fd, buffer, BUFFER_SIZE);
        if (readBytes == 0) { return 1;}
        nBytes -= readBytes;
        fwrite(buffer, readBytes, 1, f);
    } 
    
    while (nBytes > 0) {
        readBytes = read(fd, buffer, nBytes);             //CHECK
        if (readBytes == 0) { return 1;}
        nBytes -= readBytes;
        fwrite(buffer, readBytes, 1, f);
    }
    printf("STDERR: ReadWrite Path: %s | Mode: %s | Missing: %d\n", path, mode, nBytes);
    fclose(f);
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
    if (n == -1) exit(2); //check error
    printf("STDERR: Size: %ld | MESSAGE SENT: %s", strlen(buffer), buffer);
}


int readAndSendFile(char* path, char* mode, int fd) {

    FILE *f;
    int size = 0;
    char *buffer, fileSize[12];  //to see if needs changing

    printf("STDERR: Sending: %d bytes (mode %s) from path %s\n", size, mode, path);
    f = fopen(path, mode);

    fseek(f, 0 , SEEK_END);
    size = ftell(f);
    fseek(f, 0 , SEEK_SET);
    printf("STDERR: Sending: %d bytes (mode %s) from path %s\n", size, mode, path);
    if (!strcmp(mode, "rb")) {
        sprintf(fileSize, " %d ", size);
        sendMessageTCP(fileSize, strlen(fileSize), fd);
    } 

    buffer = (char*) malloc(sizeof(char)*(size+1)); 
    if (fread(buffer, size, 1, f) == -1) exit(2); //check error
    buffer[size] = '\0';
    
    fclose(f);

    sendMessageTCP(buffer, size, fd);
    free(buffer);

    return 0;
}

void sendMessageTCP(char* buffer, int size, int fdTCP) {

    int n;

    printf("STDERR: Size: %d | MESSAGE to ben sent TCP: %s\n", size, buffer);
    n = write(fdTCP, buffer, size);
    if (n == -1) exit(2);
    printf("STDERR: Size: %d | MESSAGE SENT TCP: %s\n", size, buffer);
}

int startUDP(char* address, char* port) {

    int fd, err;
    struct addrinfo hints, *res;
    struct sockaddr_in addr;
    socklen_t addrlen;
        
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_PASSIVE|AI_NUMERICSERV;

    if ((err = getaddrinfo(address, port, &hints, &res)) != 0) exit(2);          //exit
    if ((fd = socket(res->ai_family, res->ai_socktype, res->ai_protocol)) == -1) exit(2);  //exit
    if (bind(fd, res->ai_addr, res->ai_addrlen) == -1) exit(2);  //exit
    
    free(res);
    return fd;
}

int startTCP(char* address, char* port) {

    int fd, err;
    struct addrinfo hints, *res;
    struct sockaddr_in addr;
    socklen_t addrlen;
    
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE|AI_NUMERICSERV;

    if ((err = getaddrinfo(address, port, &hints, &res)) != 0) exit(2);     //exit
    if ((fd = socket(res->ai_family, res->ai_socktype, res->ai_protocol)) == -1) exit(2);       //exit
    if (bind(fd, res->ai_addr, res->ai_addrlen) == -1) exit(2);     //exit
    if (listen(fd, 5) == -1) exit(2);       //exit

    free(res);
    return fd;
}