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
	int fd, newfd;
	ssize_t n;
	socklen_t addrlen;
	struct addrinfo hints,*res;
	struct sockaddr_in addr;
	char buffer[128];

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_INET;			// IPv4
	hints.ai_socktype = SOCK_STREAM;	// TCP socket
	hints.ai_flags = AI_PASSIVE|AI_NUMERICSERV;

	n = getaddrinfo(NULL, PORT, &hints, &res);
	if (n != 0) /*error*/ exit(1);

	fd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
	if (fd == -1) /*error*/ exit(1);

	n = bind(fd, res->ai_addr, res->ai_addrlen);
	if (n == -1) /*error*/ exit(1);

	if (listen(fd, 5) == -1) /*error*/ exit(1);

	if ((newfd = accept(fd, (struct sockaddr*) &addr, &addrlen)) == -1) /*error*/ exit(1);

	n = read(newfd, buffer, 128);
	if (n == -1) /*error*/ exit(1);

	write(1, "received: ", 10);
	write(1, buffer, n);

	n = write(newfd, buffer, n);
	if (n == -1) /*error*/ exit(1);

	freeaddrinfo(res);
	close(newfd);
	close (fd);

	return 0;
}