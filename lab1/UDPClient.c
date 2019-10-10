#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <string.h>
#define PORT "58020"

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

	n = getaddrinfo("localhost", PORT, &hints, &res);
	if (n != 0) /*error*/ exit(1);

	fd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
	if (fd == -1) /*error*/ exit(1);


	n = sendto(fd, "REG\n", 7, 0, res->ai_addr, res->ai_addrlen);
	if (n == -1) /*error*/ exit(1);

	addrlen = sizeof(addr);
	n = recvfrom(fd, buffer, 128, 0, (struct sockaddr*) &addr, &addrlen);
	if (n == -1) /*error*/ exit(1);

	write(1, "echo: ", 6);
	write(1, buffer, n);

	n = sendto(fd, "reg\n", 1111, 0, res->ai_addr, res->ai_addrlen);
	if (n == -1) /*error*/ exit(1);

	addrlen = sizeof(addr);
	n = recvfrom(fd, buffer, 128, 0, (struct sockaddr*) &addr, &addrlen);
	if (n == -1) /*error*/ exit(1);

	write(1, "echo: ", 6);
	write(1, buffer, n);

		n = sendto(fd, "REG   \n", 700, 0, res->ai_addr, res->ai_addrlen);
	if (n == -1) /*error*/ exit(1);

	addrlen = sizeof(addr);
	n = recvfrom(fd, buffer, 128, 0, (struct sockaddr*) &addr, &addrlen);
	if (n == -1) /*error*/ exit(1);

	write(1, "echo: ", 6);
	write(1, buffer, n);

		n = sendto(fd, "REG 1S111\n", 700, 0, res->ai_addr, res->ai_addrlen);
	if (n == -1) /*error*/ exit(1);

	addrlen = sizeof(addr);
	n = recvfrom(fd, buffer, 128, 0, (struct sockaddr*) &addr, &addrlen);
	if (n == -1) /*error*/ exit(1);

	write(1, "echo: ", 6);
	write(1, buffer, n);

		n = sendto(fd, "REG AA\n", 700, 0, res->ai_addr, res->ai_addrlen);
	if (n == -1) /*error*/ exit(1);

	addrlen = sizeof(addr);
	n = recvfrom(fd, buffer, 128, 0, (struct sockaddr*) &addr, &addrlen);
	if (n == -1) /*error*/ exit(1);

	write(1, "echo: ", 6);
	write(1, buffer, n);

		n = sendto(fd, "REG 233\n", 700, 0, res->ai_addr, res->ai_addrlen);
	if (n == -1) /*error*/ exit(1);

	addrlen = sizeof(addr);
	n = recvfrom(fd, buffer, 128, 0, (struct sockaddr*) &addr, &addrlen);
	if (n == -1) /*error*/ exit(1);

	write(1, "echo: ", 6);
	write(1, buffer, n);

		n = sendto(fd, "REG 11111 \n", 700, 0, res->ai_addr, res->ai_addrlen);
	if (n == -1) /*error*/ exit(1);

	addrlen = sizeof(addr);
	n = recvfrom(fd, buffer, 128, 0, (struct sockaddr*) &addr, &addrlen);
	if (n == -1) /*error*/ exit(1);

	write(1, "echo: ", 6);
	write(1, buffer, n);

		n = sendto(fd, "REG 99999 SFSDFSDFSDF\n", 700, 0, res->ai_addr, res->ai_addrlen);
	if (n == -1) /*error*/ exit(1);

	addrlen = sizeof(addr);
	n = recvfrom(fd, buffer, 128, 0, (struct sockaddr*) &addr, &addrlen);
	if (n == -1) /*error*/ exit(1);

	write(1, "echo: ", 6);
	write(1, buffer, n);

		n = sendto(fd, "REG 23232\n", 700, 0, res->ai_addr, res->ai_addrlen);
	if (n == -1) /*error*/ exit(1);

	addrlen = sizeof(addr);
	n = recvfrom(fd, buffer, 128, 0, (struct sockaddr*) &addr, &addrlen);
	if (n == -1) /*error*/ exit(1);

	write(1, "echo: ", 6);
	write(1, buffer, n);

		n = sendto(fd, "REG 22222 22222\n", 700, 0, res->ai_addr, res->ai_addrlen);
	if (n == -1) /*error*/ exit(1);

	addrlen = sizeof(addr);
	n = recvfrom(fd, buffer, 128, 0, (struct sockaddr*) &addr, &addrlen);
	if (n == -1) /*error*/ exit(1);

	write(1, "echo: ", 6);
	write(1, buffer, n);

	n = sendto(fd, "REG 12312 SDF SDFS DF SDF SDF\n", 700, 0, res->ai_addr, res->ai_addrlen);
	if (n == -1) /*error*/ exit(1);

	addrlen = sizeof(addr);
	n = recvfrom(fd, buffer, 128, 0, (struct sockaddr*) &addr, &addrlen);
	if (n == -1) /*error*/ exit(1);

	write(1, "echo: ", 6);
	write(1, buffer, n);
	
	n = sendto(fd, "LTP\n", 700, 0, res->ai_addr, res->ai_addrlen);
	if (n == -1) /*error*/ exit(1);

	addrlen = sizeof(addr);
	n = recvfrom(fd, buffer, 128, 0, (struct sockaddr*) &addr, &addrlen);
	if (n == -1) /*error*/ exit(1);

	write(1, "echo: ", 6);
	write(1, buffer, n);
	
	
	n = sendto(fd, "LTP \n", 700, 0, res->ai_addr, res->ai_addrlen);
	if (n == -1) /*error*/ exit(1);

	addrlen = sizeof(addr);
	n = recvfrom(fd, buffer, 128, 0, (struct sockaddr*) &addr, &addrlen);
	if (n == -1) /*error*/ exit(1);

	write(1, "echo: ", 6);
	write(1, buffer, n);

	
	n = sendto(fd, "LTP    \n", 700, 0, res->ai_addr, res->ai_addrlen);
	if (n == -1) /*error*/ exit(1);

	addrlen = sizeof(addr);
	n = recvfrom(fd, buffer, 128, 0, (struct sockaddr*) &addr, &addrlen);
	if (n == -1) /*error*/ exit(1);

	write(1, "echo: ", 6);
	write(1, buffer, n);

	
	n = sendto(fd, "LTP SADASDA\n", 700, 0, res->ai_addr, res->ai_addrlen);
	if (n == -1) /*error*/ exit(1);

	addrlen = sizeof(addr);
	n = recvfrom(fd, buffer, 128, 0, (struct sockaddr*) &addr, &addrlen);
	if (n == -1) /*error*/ exit(1);

	write(1, "echo: ", 6);
	write(1, buffer, n);

	
	n = sendto(fd, "PTP SADASDA\n", 700, 0, res->ai_addr, res->ai_addrlen);
	if (n == -1) /*error*/ exit(1);

	addrlen = sizeof(addr);
	n = recvfrom(fd, buffer, 128, 0, (struct sockaddr*) &addr, &addrlen);
	if (n == -1) /*error*/ exit(1);

	write(1, "echo: ", 6);
	write(1, buffer, n);

	
	n = sendto(fd, "PTP 12333 SADDD\n", 700, 0, res->ai_addr, res->ai_addrlen);
	if (n == -1) /*error*/ exit(1);

	addrlen = sizeof(addr);
	n = recvfrom(fd, buffer, 128, 0, (struct sockaddr*) &addr, &addrlen);
	if (n == -1) /*error*/ exit(1);

	write(1, "echo: ", 6);
	write(1, buffer, n);

	
	n = sendto(fd, "PTP 12333 SADDD\n", 700, 0, res->ai_addr, res->ai_addrlen);
	if (n == -1) /*error*/ exit(1);

	addrlen = sizeof(addr);
	n = recvfrom(fd, buffer, 128, 0, (struct sockaddr*) &addr, &addrlen);
	if (n == -1) /*error*/ exit(1);

	write(1, "echo: ", 6);
	write(1, buffer, n);
		
	n = sendto(fd, "PTP\n", 700, 0, res->ai_addr, res->ai_addrlen);
	if (n == -1) /*error*/ exit(1);

	addrlen = sizeof(addr);
	n = recvfrom(fd, buffer, 128, 0, (struct sockaddr*) &addr, &addrlen);
	if (n == -1) /*error*/ exit(1);

	write(1, "echo: ", 6);
	write(1, buffer, n);

		
	n = sendto(fd, "PTP 34443 SSDD\n", 700, 0, res->ai_addr, res->ai_addrlen);
	if (n == -1) /*error*/ exit(1);

	addrlen = sizeof(addr);
	n = recvfrom(fd, buffer, 128, 0, (struct sockaddr*) &addr, &addrlen);
	if (n == -1) /*error*/ exit(1);

	write(1, "echo: ", 6);
	write(1, buffer, n);

		
	n = sendto(fd, "PTP ASDF SADDD\n", 700, 0, res->ai_addr, res->ai_addrlen);
	if (n == -1) /*error*/ exit(1);

	addrlen = sizeof(addr);
	n = recvfrom(fd, buffer, 128, 0, (struct sockaddr*) &addr, &addrlen);
	if (n == -1) /*error*/ exit(1);

	write(1, "echo: ", 6);
	write(1, buffer, n);

	n = sendto(fd, "PTP \n", 700, 0, res->ai_addr, res->ai_addrlen);
	if (n == -1) /*error*/ exit(1);

	addrlen = sizeof(addr);
	n = recvfrom(fd, buffer, 128, 0, (struct sockaddr*) &addr, &addrlen);
	if (n == -1) /*error*/ exit(1);

	write(1, "echo: ", 6);
	write(1, buffer, n);

	n = sendto(fd, "PTP    \n", 700, 0, res->ai_addr, res->ai_addrlen);
	if (n == -1) /*error*/ exit(1);

	addrlen = sizeof(addr);
	n = recvfrom(fd, buffer, 128, 0, (struct sockaddr*) &addr, &addrlen);
	if (n == -1) /*error*/ exit(1);

	write(1, "echo: ", 6);
	write(1, buffer, n);

	n = sendto(fd, "PTP 222222 ASDFADSF\n", 700, 0, res->ai_addr, res->ai_addrlen);
	if (n == -1) /*error*/ exit(1);

	addrlen = sizeof(addr);
	n = recvfrom(fd, buffer, 128, 0, (struct sockaddr*) &addr, &addrlen);
	if (n == -1) /*error*/ exit(1);

	write(1, "echo: ", 6);
	write(1, buffer, n);
	n = sendto(fd, "PTP 22222 ADFASDFADSF\n", 700, 0, res->ai_addr, res->ai_addrlen);
	if (n == -1) /*error*/ exit(1);

	addrlen = sizeof(addr);
	n = recvfrom(fd, buffer, 128, 0, (struct sockaddr*) &addr, &addrlen);
	if (n == -1) /*error*/ exit(1);

	write(1, "echo: ", 6);
	write(1, buffer, n);
	n = sendto(fd, "PTP 222DF222 ADFADSF\n", 700, 0, res->ai_addr, res->ai_addrlen);
	if (n == -1) /*error*/ exit(1);

	addrlen = sizeof(addr);
	n = recvfrom(fd, buffer, 128, 0, (struct sockaddr*) &addr, &addrlen);
	if (n == -1) /*error*/ exit(1);

	write(1, "echo: ", 6);
	write(1, buffer, n);

	n = sendto(fd, "LTP\n", 700, 0, res->ai_addr, res->ai_addrlen);
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