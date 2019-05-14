#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/stat.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <fcntl.h>
#define PORT 0x0da2
#define IP_ADDR 0x7f000001
#define FILENAME "coolfile.txt"
int ctoi(char c) 
{

    return c-'0';

}


int main(void)
{
	int sock = socket(AF_INET, SOCK_STREAM, 0);
	ssize_t nrecv,nread, nwrite, i;
	size_t strsize;
	struct sockaddr_in s = {0};
	s.sin_family = AF_INET;
	s.sin_port = htons(PORT);
	s.sin_addr.s_addr = htonl(IP_ADDR);

	if (connect(sock, (struct sockaddr*)&s, sizeof(s)) < 0)
	{
		perror("connect");
		return 1;
	}

	char buffer[1000] = {0};

	if ((nrecv = recv(sock, &strsize, sizeof(int), 0)) < 0)
	{
		perror("recv");
		return 1;
	}


	if ((nrecv = recv(sock, &buffer, strsize, 0)) < 0)
	{
		perror("recv");
		return 1;
	}

	printf("./client list-files\n%s\n", buffer);
	int fd;
	struct stat file_stat;
	int numcheck = 0;
	char namebuff[100] = FILENAME;
	fd=open(FILENAME,O_RDONLY);
	if (fd == -1)
	{
		perror("open");
		return 1;
	}
	strsize = strlen(namebuff);
	printf("./client %s\n", namebuff);

	if (send(sock, &strsize, sizeof(int), 0) < 0)
		{
			perror("send");
			return 1;
		}

	if (send(sock, &namebuff, strlen(namebuff), 0) < 0)
		{
			perror("send");
			return 1;
		}




	if ((nrecv = recv(sock, &numcheck, sizeof(int), 0)) < 0)
	{
		perror("recv");
		return 1;
	}

	if (numcheck == -1)
	{
		printf("file already exists in the server\n");
		// used here printf and no perror as this not returns an error, 
		// and will send to the client sucsess as it's not an in built function in C
		return 1;
	}

	if (fstat(fd, &file_stat) < 0)
       	{
		perror("error");
               	return 1;
        }

	strsize=lseek(fd,0,SEEK_END);
	
	if(strsize<0)
	{
		perror("lseek");
		return 1;
	}

	if((lseek(fd, 0, SEEK_SET)) <0)
	{
		perror("lseek");
		return 1;
	}

	if(send(sock, &strsize, sizeof(int), 0) <0)
        {
              	perror("send");
              	return 1;
        }


	char* buf = (char*) malloc(strsize + 1);
	memset(buf, 0 , strsize + 1);
	nread = 1;
	while(nread != 0)
	{
		nread = read(fd, buf , strsize + 1);
		for(i=0; i < nread; i += nwrite)
			nwrite = send(sock, buf + i, nread - i , 0);
				
	}
	free(buf);
	close(sock);
	printf("file %s has been successfully uploaded to the server.\n", namebuff);
	return 0;
}
