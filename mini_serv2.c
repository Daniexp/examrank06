#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <stdio.h>

fd_set allfd, readfd, writefd;
int id[1027];
char* msg[1027];
int fds, clientId; 

int extract_message(char **buf, char **msg)
{
	char	*newbuf;
	int	i;

	*msg = 0;
	if (*buf == 0)
		return (0);
	i = 0;
	while ((*buf)[i])
	{
		if ((*buf)[i] == '\n')
		{
			newbuf = calloc(1, sizeof(*newbuf) * (strlen(*buf + i + 1) + 1));
			if (newbuf == 0)
				return (-1);
			strcpy(newbuf, *buf + i + 1);
			*msg = *buf;
			(*msg)[i + 1] = 0;
			*buf = newbuf;
			return (1);
		}
		i++;
	}
	return (0);
}

char *str_join(char *buf, char *add)
{
	char	*newbuf;
	int		len;

	if (buf == 0)
		len = 0;
	else
		len = strlen(buf);
	newbuf = malloc(sizeof(*newbuf) * (len + strlen(add) + 1));
	if (newbuf == 0)
		return (0);
	newbuf[0] = 0;
	if (buf != 0)
		strcat(newbuf, buf);
	free(buf);
	strcat(newbuf, add);
	return (newbuf);
}

void error(char *msg)
{
	if (!msg)
		msg = "Fatal error";
	write(2, msg, strlen(msg));
	write(2, "\n", 1);
}

int sendToAll(const int sender, char *msg)
{
	int res = 1;
	for (int i = 0; i <= fds; i++)
	{
		if (i != sender && FD_ISSET(i, &writefd))
			if (send(i, msg, strlen(msg), 0) == -1)
				res = 0;
	}
	return res;
}

int main(int argc, char **argv) {
	if (argc < 2)
		error(NULL);
	int sockfd, connfd;
	unsigned int len;
	struct sockaddr_in servaddr, cli; 
	fds = clientId = 0;

	// socket create and verification 
	sockfd = socket(AF_INET, SOCK_STREAM, 0); 
	if (sockfd == -1)
		error(NULL);
	bzero(&servaddr, sizeof(servaddr)); 

	//Add socket to select
	fds = sockfd;
	FD_ZERO(&allfd);
	FD_SET(sockfd, &allfd);
	// assign IP, PORT 
	servaddr.sin_family = AF_INET; 
	servaddr.sin_addr.s_addr = htonl(2130706433); //127.0.0.1
	servaddr.sin_port = htons(atoi(argv[1])); 
  
	// Binding newly created socket to given IP and verification 
	if ((bind(sockfd, (const struct sockaddr *)&servaddr, sizeof(servaddr))) != 0)
		error(NULL);
	if (listen(sockfd, 100) != 0)
		error(NULL);
	char buf[1001];
	char endMsg[2001];
	while (1)
	{
		bzero(buf, 1001);
		readfd = writefd = allfd;
		if (select(fds + 1, &readfd, &writefd, 0, 0) == -1)
			continue ;
		for (int i = 0; i <= fds; i++)
		{
			if (FD_ISSET(i, &readfd) == 0)
				continue ;
			if (i == sockfd)
			{
				connfd = accept(sockfd, (struct sockaddr *)&cli, &len);
				if (connfd == -1)
					error(NULL); 
				if (fds < connfd)
					fds = connfd;
				//New client
				id[connfd] = clientId++;
				FD_SET(connfd, &allfd);
				//SendMessage
				sprintf(buf, "server: client %d just arrived\n", id[connfd]);
				sendToAll(connfd, buf);
				printf("New connection to server\n");
			}
			else
			{
				int len = recv(i, buf, 1000, 0);
				if (len <= 0)
				{
					printf("New Disconnection from server\n");
					FD_CLR(i, &allfd);
					sprintf(buf, "server: client %d just left\n", id[i]);
					sendToAll(i, buf);
					id[i] = -1;
					if (msg[i])
						free(msg[i]);
					msg[i] = NULL;
				}
				else
				{
					printf("We receive something: %d\n", len);
					printf("buf: %s", buf);
					
					buf[len] = '\0';
					msg[i] = str_join(msg[i], buf);
					char *message;
					while (extract_message(&msg[i], &message))
					{
						sprintf(endMsg, "client %d: %s", id[i], message);
						sendToAll(i, endMsg);
						free(message);
					}
				}
				//Client send message or disconnect from the server
			}
		}
	}
}
