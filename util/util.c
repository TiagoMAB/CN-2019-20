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

#define PORT "58020"
#define MAX_TOPICS 99
#define TOPIC_SIZE 17
#define BUFFER_SIZE 2048
#define MAX_PATH_SIZE 57

int checkUser(char* id) {   	    //checkED

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

int dirSize(char* path) {       //checked

    DIR *d;
    struct dirent *dir;
    int size = 0;

    d = opendir(path);
    if (d) {
        while ((dir = readdir(d)) != NULL) {
            if ((dir->d_type == DT_DIR) && strcmp(dir->d_name, ".") && strcmp(dir->d_name, "..")) {
                size++;                     
            }
        }
        if (closedir(d)) { printf("There was an error closing a directory\n"); exit(1);}                          
        return size;
    }
    else {
        printf("There was an error opening a directory\n"); 
        exit(1);
    }
}

int verifyName(char* name) {        //checked

    if (name != NULL && strlen(name) <= 10) {
        for (int i = 0; i < strlen(name); i++) {
            if ((name[i] < '0' || name[i] > '9') && (name[i] < 'A' || name[i] > 'Z') && (name[i] < 'a' || name[i] > 'z')) {
                return 1;
            }
        }
        return 0;
    }
    return 1; 
}

int readAndWrite(char *path, char* mode, int nBytes, int fd) {

    int readBytes = 0;
    char* buffer[BUFFER_SIZE+1];
    char* size, *end;
    FILE *f;

    if ((f = fopen(path, mode)) == NULL ) { return 1; }

    if (nBytes == 0) {
        size = readToken(size, fd, 1);
        if (size[0] == '\0' || (size[strlen(size)-1] == '\n') || strlen(size) > 10) {
            free(size); 
            return 1; 
        }
        if (!(nBytes = strtol(size, &end, 10)) || end[0] != '\0') {
            free(size);
            return 1;
        } 
    }

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
    free(size);
    if (fclose(f) == EOF) { return 1; }; 
    return 0;

}

char* sendAndReadUDP(int fd, struct addrinfo *res, char* request, char* answer) {

    socklen_t addrlen;          
    struct sockaddr_in addr;
    int n = 0, data = 0, attempts = 0;
    char* buffer = (char*) malloc(sizeof(char)*BUFFER_SIZE);
    
    n = sendto(fd, request, strlen(request), 0, res->ai_addr, res->ai_addrlen);
    if (n == -1) { 
        printf("There was a problem with UDP connection\n"); exit(1);
    }
    
    sleep(1);
    addrlen = sizeof(addr);

    data = recvfrom(fd, buffer, BUFFER_SIZE - 1, MSG_DONTWAIT, (struct sockaddr*)&addr, &addrlen);
    while (data == -1 && attempts < 4)  {
        attempts++;
        printf("Server not responding, sending new request\n"); 
        sleep(4);

        n = sendto(fd, request, strlen(request), 0, res->ai_addr, res->ai_addrlen);
        if (n == -1) { 
            printf("There was a problem with UDP connection\n"); exit(1);
        }
    
        data = recvfrom(fd, buffer, BUFFER_SIZE - 1, MSG_DONTWAIT, (struct sockaddr*)&addr, &addrlen);
    }

    if (data != -1) {
        buffer[data] = '\0';
        return buffer;
    }
    else {
        printf("Server not responding, aborting request\n"); 
        free(buffer); return NULL;
    }
}

int sendMessageUDP(char* buffer, int fdUDP, struct sockaddr_in addr, socklen_t addrlen) {

    int n;
    addrlen = sizeof(addr);
    
    n = sendto(fdUDP, buffer, strlen(buffer), 0, (struct sockaddr*)&addr, addrlen);
    if (n == -1)  { return -1; }; 

    return 0;
}

int readAndSend(char* path, char* mode, int fd) {

    FILE *f;
    int size = 0;
    char *buffer, fileSize[13];  //to see if needs changing

    if ((f = fopen(path, mode)) == NULL ) { return 1; }

    fseek(f, 0 , SEEK_END);
    size = ftell(f);
    fseek(f, 0 , SEEK_SET);

    if (!strcmp(mode, "rb")) {
        sprintf(fileSize, " %d ", size);
        if (strlen(fileSize) > 12) { return 1; }
        sendMessageTCP(fileSize, strlen(fileSize), fd);
    } 

    buffer = (char*) malloc(sizeof(char)*(size+1)); 
    if (fread(buffer, size, 1, f) == -1) exit(2); //check error
    buffer[size] = '\0';
    
    if (fclose(f) == EOF) { return 1; }; 

    sendMessageTCP(buffer, size, fd);
    free(buffer);

    return 0;
}

int sendMessageTCP(char* buffer, int size, int fd) {            //checked
    
    int n;

    while (size > 0) {
        n = write(fd, buffer, size);
        if (n == -1) { return 1; }
        size -= n;
    }
    return 0;
}

int saveFolder(int fd, char* user, char* name, char* path) {  //checked

    char pathUID[MAX_PATH_SIZE] = "", pathTitle[MAX_PATH_SIZE] = "", pathIMG[MAX_PATH_SIZE] = "";
    char *flag = NULL, *ext = NULL;
    FILE *f;

    if (mkdir(path, 0777) == -1 && errno != EEXIST) { return 1; }

    sprintf(pathUID, "%s/%s_UID.txt", path, name);

    printf("%s\n", pathUID);
    if ((f = fopen(pathUID, "w")) == NULL ) { return 1; }
    fwrite(user, 1, strlen(user), f);                                     
    if (fclose(f) == EOF) { return 1; };      

     printf("%s\n", pathTitle);
    sprintf(pathTitle, "%s/%s.txt", path, name);
    if (readAndWrite(pathTitle, "w", 0, fd)) { return 1; }

    free(readToken(NULL, fd, 0));
    flag = readToken(flag, fd, 1);
    if (!strcmp(flag, "1")) {
        ext = readToken(ext, fd, 0);
        if (ext[0] == '\0' || ext[strlen(ext)-1] == '\n') { free(ext); free(flag); return 1; }
        sprintf(pathIMG, "%s/%s.%s", path, name, ext);
        if (readAndWrite(pathIMG, "wb", 0, fd)) { free(ext); free(flag); return 1; }
    }
    else if (!strcmp(flag, "0\n")) { free(flag); return 0; }
    else if (!strcmp(flag, "0")) { free(flag); return 2; }
    else { free(flag); return 1; }

    free(flag);
    flag = readToken(flag, fd, 1);
    if (!strcmp(flag, "\n")){
        free(ext); free(flag);
        return 0;
    }
    else if (flag[0] == '\0') {
        free(ext); free(flag);
        return 2;
    }

    free(flag); free(ext);
    return 1;
}

int startUDP(char* address, char* port) {   //checked

    int fd, err;
    struct addrinfo hints, *res;
    struct sockaddr_in addr;
    socklen_t addrlen;
        
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_PASSIVE|AI_NUMERICSERV;

    if ((err = getaddrinfo(address, port, &hints, &res)) != 0) { return -1; }   
    if ((fd = socket(res->ai_family, res->ai_socktype, res->ai_protocol)) == -1) { return -1; }  
    if (bind(fd, res->ai_addr, res->ai_addrlen) == -1) { return -1; }  
    
    freeaddrinfo(res);
    return fd;
}

int startTCP(char* address, char* port, int flag) {     //checked

    int fd, err;
    struct addrinfo hints, *res;
    struct sockaddr_in addr;
    socklen_t addrlen;
    
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE|AI_NUMERICSERV;

    if ((err = getaddrinfo(address, port, &hints, &res)) != 0) { exit(1); }  
    if ((fd = socket(res->ai_family, res->ai_socktype, res->ai_protocol)) == -1) { exit(1);; }      
    
    if (flag) {
        if (bind(fd, res->ai_addr, res->ai_addrlen) == -1) { exit(1); }   
        if (listen(fd, 5) == -1) { exit(1); }      
    }
    else {
        if (connect(fd, res->ai_addr, res->ai_addrlen) == -1) { exit(1); }  
    }

    freeaddrinfo(res);
    return fd;
}

int checkDir(char* path) { //checked

    DIR *d;

    d = opendir(path);
    if (d) { 
        if (closedir(d)) { printf("There was an error closing a directory\n"); exit(1);}
        return 1; 
    }
    else if (ENOENT == errno) { return 0;}
    else { return 1; } 
}

int readAndSave(int fd, char* path, char* question) {       //checked

    char *id = readToken(id, fd, 1), ret = 0;

    if (id[0] == '\0' || id[strlen(id)-1] == '\n') {
        free(id);
        return 1;
    }

    ret = saveFolder(fd, id, question, path);

    free(id);
    return ret;
}

char* readMessageUDP(char* buffer, int fdUDP, struct sockaddr *addr, socklen_t *addrlen) {

    int size = 4, data = 4;

    buffer = (char*) malloc(sizeof(char)*size);
    *addrlen = sizeof(addr);

    do {
        free(buffer);
        size *= 2;
        
        buffer = (char*) malloc(sizeof(char)*size);
        data = recvfrom(fdUDP, buffer, size - 1, MSG_PEEK, addr, addrlen);
        if (data == -1) {
            printf("There was a problem with UDP connection\n"); exit(1);
        }

        buffer[size-1] = '\0'; 
    } while (strlen(buffer) == size - 1);

    free(buffer);
    buffer = (char*) malloc(sizeof(char)*(data+1));
    data = recvfrom(fdUDP, buffer, data, 0, addr, addrlen);
    if (data == -1) {
        printf("There was a problem with UDP connection\n"); exit(1);
    }
    buffer[data] = '\0';

    return buffer;
}

char* readToken(char* answer, int fdTCP, int flag) {

    int readBytes = 0, size = 2;
    char buffer[2] = "";
    answer = (char*) malloc(sizeof(char)*size);
    memset(answer, '\0', size);

    while (readBytes = read(fdTCP, buffer, 1) && buffer[0] != ' ' && (buffer[0] != '\n' || flag)) {     //check for 
        buffer[1] = '\0';
        size++;
        answer = (char*) realloc(answer, sizeof(char)*size);
        strcat(answer, buffer);
        if (flag && buffer[0] == '\n') { return answer; }
    }

    return answer;
}
