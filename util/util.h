#ifndef UTIL_H
#define UTIL_H


int dirSize(char* path) ;

int checkUser(char* id);

int checkDir(char* topic);

int verifyName(char* topic);

char* readToken(char* answer, int fdTCP, int flag);

int readAndWrite(char *path, char* mode, int nBytes, int fd);

char* readMessageUDP(char* buffer, int fdUDP, struct sockaddr *addr, socklen_t *addrlen);

int sendMessageUDP(char* buffer, int fdUDP, struct sockaddr_in addr, socklen_t addrlen);

int readAndSend(char* path, char* mode, int fd);

int sendMessageTCP(char* buffer, int size, int fdTCP);

int startUDP(char* address, char* port);

int startTCP(char* address, char* port, int flag);

int checkDir(char* path);

int saveFolder(int fd, char* user, char* name, char* path);

int readAndSave(int fd, char* path, char* question);
#endif