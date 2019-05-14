#include <stdio.h>
#include <stdlib.h>
#include <time.h>
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
#include <dirent.h>
#include <fcntl.h>
#include <sys/select.h> 

#define PORT 0x0da2
#define IP_ADDR 0x7f000001
#define QUEUE_LEN 20

int main(void)
{
	DIR* directory;
	struct dirent* ent;
	int fd;
	fd_set father;
	fd_set fdread;
	pid_t pid;
	size_t strsize;
	struct stat fileStat;
	
	int listenS = socket(AF_INET, SOCK_STREAM, 0);

	if (listenS < 0)
	{
		perror("socket");
		return 1;
	}

	struct sockaddr_in s = {0};
	s.sin_family = AF_INET;
	s.sin_port = htons(PORT);
	s.sin_addr.s_addr = htonl(IP_ADDR);

	if (bind(listenS, (struct sockaddr*)&s, sizeof(s)) < 0)
	{
		perror("bind");
		return 1;
	}

	if (listen(listenS, QUEUE_LEN) < 0)
	{
		perror("listen");
		return 1;
	}

	struct sockaddr_in clientIn;
	int clientInSize = sizeof clientIn;

	while (1)
	{
		int newfd = accept(listenS, (struct sockaddr*)&clientIn, (socklen_t*)&clientInSize);
		if (newfd < 0)
		{
			perror("accept");
			return 1;
		}
		if((pid = fork()) == -1)
		{
			close(newfd);
			continue;
		}
		else if(pid > 0)
		{
			close(newfd);
			continue;
		}

		else if(pid == 0)
		{
			char buffer[1000] = {0};
			char convert[100] = {0};
			if ((directory = opendir ("server")) == NULL) 
			{
        			perror ("Cannot open .");
        			return 1;
    			}

			while ((ent = readdir(directory)) != NULL) 
			{

				if (!( !strcmp(ent->d_name, ".") || !strcmp(ent->d_name, "..")) )
				{
					fd = openat(dirfd(directory), ent->d_name, 0);

					if (fd == -1)
					{
        					perror ("Can't get stats.");
        					return 1;
    					}	

					if(fstat(fd, &fileStat)== -1)
					{
        					perror ("Can't get stats.");
        					return 1;
    					}

					strcat(buffer,ent->d_name);
					sprintf(convert,"%lld", (long long) fileStat.st_size);
					strcat(buffer, " ");
					strcat(buffer,convert);
					strcat(buffer,"\n");
				
				}
    			}
			closedir(directory);

			if (newfd < 0)
			{
				perror("accept");
				return 1;
			}
			strsize = strlen(buffer);
			if (send(newfd, &strsize, sizeof(int), 0) < 0)
			{
				perror("send");
				return 1;
			}
			if (send(newfd, &buffer, strsize, 0) < 0)
			{
				perror("send");
				return 1;
			}
		
			char namebuff[100] = {0};
			ssize_t nrecv;
			int error = 0;
			if ((nrecv = recv(newfd, &strsize, sizeof(int), 0)) < 0)
			{
				perror("recv");
				return 1;
			}
			if ((nrecv = recv(newfd, &namebuff, strsize, 0)) < 0)
			{
				perror("recv");
				return 1;
			}

			if((strstr(buffer,namebuff) != NULL))
			{
				error = -1;

				if (send(newfd, &error, sizeof(int), 0) < 0)
				{
					perror("send");
					return 1;
				}

			}
			else
			{
				if (send(newfd, &error, sizeof(int), 0) < 0)
				{
					perror("send");
					return 1;
				}
			}

			char pathy[100] = "server/";
			strcat(pathy,namebuff);
			if(error != -1) 
			{

				if ((nrecv = recv(newfd, &strsize, sizeof(int), 0)) < 0)
				{
					perror("recv");
					return 1;
				}
				ssize_t count = 1;
            			ssize_t total = 0;
            			char* sendbuffer = (char*) malloc(strsize);
				if (sendbuffer == NULL)
				{
					perror("malloc");
					return 1;
				}
            			memset(sendbuffer, 0, strsize);
            			FD_ZERO(&father);
            			FD_ZERO(&fdread);
            			FD_SET(0,&father);
            			FD_SET(newfd,&father);

            			while(count != 0)
				{
                			fdread = father;
                			if (select(newfd + 1,&fdread,NULL,NULL,NULL) == -1)
					{
                				perror("select");
                				exit(1);
                			}

               				if (FD_ISSET(newfd, &fdread))
					{
                			count = recv(newfd, sendbuffer + total, strsize - total, 0);
                			total += count;
                			}
            			}
                		int newfile = open(pathy, O_RDWR|O_CREAT, 0666);

                		if(newfile < 0)
				{
                    			perror("open");
                    			return 1;
				}

                    		write(newfile, sendbuffer, strsize);
				close(newfile);
                    		close(newfd);
				free(sendbuffer);
				continue;	
			}	
		}
		close(listenS);
		return 0;
	}
}
