#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#define PORT "58020"

void error(int error) {

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

char* readSocket(int fdUDP, char* buffer, struct sockaddr_in addr, socklen_t addrlen) {

    int size = 1, data;

    buffer = (char*) malloc(sizeof(char)*size);
    addrlen = sizeof(addr);

    data = recvfrom(fdUDP, buffer, size, MSG_PEEK, (struct sockaddr*)&addr, &addrlen);
    while (data == size) {
        free(buffer);
        size *= 2;
        buffer = (char*) malloc(sizeof(char)*size);
        data = recvfrom(fdUDP, buffer, size, MSG_PEEK, (struct sockaddr*)&addr, &addrlen);
    }
    buffer[data] = '\0';
    
    return buffer;
}

char* handleMessage(char* buffer) {

    
}

int main(int argc, char **argv) {

    int option, p = 0, fdUDP, fdTCP, err;
    char *fsport = PORT, *buffer;
    struct addrinfo hints, *resUDP, *resTCP;
    struct sockaddr_in addr;
    socklen_t addrlen;
    ssize_t n, nread;
    
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
    printf("%d, %d\n", optind, argc);
    if (optind < argc) error(1);

    //Initializing UDP socket
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_PASSIVE|AI_NUMERICSERV;

    if ((err = getaddrinfo(NULL, fsport, &hints, &resUDP)) != 0) error(2);
    if ((fdUDP = socket(resUDP->ai_family, resUDP->ai_socktype, resUDP->ai_protocol)) == -1) error(2);
    if (bind(fdUDP, resUDP->ai_addr, resUDP->ai_addrlen) == -1) error(2);

    while (1) { 
        buffer = readMessageUDP(fdUDP, buffer, addr, addrlen);
        buffer = handleMessage(buffer);
        sendMessageUDP();
        printf("| %s %ld\n", buffer, strlen(buffer));
        free(buffer);
    }
    close(fdUDP);
    return 0;
}

/*
int main(void)
{
struct addrinfo hints,*res;
int fd,errcode;
struct sockaddr_in addr;
socklen_t addrlen;
ssize_t n,nread;
char buffer[128];
memset(&hints,0,sizeof hints);
hints.ai_family=AF_INET;//IPv4
hints.ai_socktype=SOCK_DGRAM;//UDP socket
hints.ai_flags=AI_PASSIVE|AI_NUMERICSERV;
if((errcode=getaddrinfo(NULL,"58001",&hints,&res))!=0)/*errorexit(1);
if((fd=socket(res->ai_family,res->ai_socktype,res->ai_protocol))==-1)/*errorexit(1);
if(bind(fd,res->ai_addr,res->ai_addrlen)==-1)/*errorexit(1);
while(1){addrlen=sizeof(addr);
nread=recvfrom(fd,buffer,128,0,(struct sockaddr*)&addr,&addrlen);
if(nread==-1)/*errorexit(1);
n=sendto(fd,buffer,nread,0,(struct sockaddr*)&addr,addrlen);
if(n==-1)/*errorexit(1);
}
//freeaddrinfo(res);
//close(fd);
//exit(0);*/


/*    if (data > 0) {
        buffer = (char*) malloc(sizeof(char)*data);
        recvfrom(fdUDP, buffer, data, MSG_TRUNC, (struct sockaddr*)&addr, &addrlen);
        printf("Message: %s", buffer);
    }
    else if ( data == 0) {
        printf("The peer has performed an orderly shutdown.\n");
        exit(1); // probs to change
    }
    else {
        printf("An error has occurred.\n");
        exit(1); //probs to change
    }
*/
