#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <string.h>
#define PORT "58000"

int main() {
	int fd;
	ssize_t n;
	socklen_t addrlen;
	struct addrinfo hints,*res;
	struct sockaddr_in addr;
	char buffer[128];

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_INET;			//IPv4
	hints.ai_socktype = SOCK_DGRAM;		//UDP socket
	hints.ai_flags = AI_NUMERICSERV;

	n = getaddrinfo("daniel-VirtualBox", PORT, &hints, &res);
	if (n != 0) /*error*/ exit(1);

	fd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
	if (fd == -1) /*error*/ exit(1);

	n = sendto(fd, "Hello!\n", 7, 0, res->ai_addr, res->ai_addrlen);
	if (n == -1) /*error*/ exit(1);

	addrlen = sizeof(addr);
	n = recvfrom(fd, buffer, 128, 0, (struct sockaddr*) &addr, &addrlen);
	if (n == -1) /*error*/ exit(1);

	write(1, "echo: ", 6);
	write(1, buffer, n);

	freeaddrinfo(res);
	close(fd);

	return 0;
}