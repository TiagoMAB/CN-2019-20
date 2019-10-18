#ifndef UTIL_H
#define UTIL_H

int checkUser(char* id);

int verifyName(char* name);

int dirSize(char* path) ;

int checkDir(char* path);

char* readToken(char* answer, int fdTCP, int flag);

int readAndWrite(char *path, char* mode, int nBytes, int fd);

int readAndSend(char* path, char* mode, int fd);

int readAndSave(int fd, char* path, char* question);

int saveFolder(int fd, char* user, char* name, char* path);

int sendMessageTCP(char* buffer, int size, int fdTCP);

char* readMessageUDP(char* buffer, int fdUDP, struct sockaddr *addr, socklen_t *addrlen);

int sendMessageUDP(char* buffer, int fdUDP, struct sockaddr_in addr, socklen_t addrlen);

int startUDP(char* address, char* port);

int startTCP(char* address, char* port, int flag);

#endif