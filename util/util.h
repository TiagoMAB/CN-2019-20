#ifndef UTIL_H
#define UTIL_H

int dirSize(char* path) ;

int checkUser(char* id);

int checkDir(char* topic);

int verifyName(char* topic);

char* readToken(char* answer, int fdTCP, int flag);

int readAndWrite(char *path, char* mode, int nBytes, int fd);

char* readMessageUDP(char* buffer, int fdUDP, struct sockaddr_in *addr, socklen_t *addrlen);

void sendMessageUDP(char* buffer, int fdUDP, struct sockaddr_in addr, socklen_t addrlen);

int readAndSendFile(char* path, char* mode, int fd);

void sendMessageTCP(char* buffer, int size, int fdTCP);

int startUDP(char* address, char* port);

int startTCP(char* address, char* port);

int checkDir(char* path);

#endif