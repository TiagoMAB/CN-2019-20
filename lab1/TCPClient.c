#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <string.h>
#include <stdio.h>
#define PORT "58020"

int main() {
	
	printf("%s", strcmp("ola", "ola") ? "nao sao\n" : "sao\n");
	return 0;
}