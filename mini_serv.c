#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <stdio.h>

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
	exit(1);
}

typedef struct  s_client{
	char		*msg;
	int		id;
} t_client;

void	sendMessage(const int max, fd_set writefd, const int sender, char *msg)
{
	for (int i = 3; i <= max; i++)
	{
		if (i != sender && FD_ISSET(i, &writefd))
			send(i, msg, strlen(msg), 0);
	}
}

int main(int argc, char **argv) {
	int sockfd, connfd;
	unsigned int len;
	struct sockaddr_in servaddr, cli; 
	fd_set allfd, readfd, writefd;
	int maxFd, maxClient;
	t_client clients[1027];
	maxClient = 0;

	if (argc <= 1)
		error("Wrong number of arguments");
	// socket create and verification 
	sockfd = socket(AF_INET, SOCK_STREAM, 0); 
	if (sockfd == -1) 
		error(NULL);
	bzero(&servaddr, sizeof(servaddr)); 
	FD_ZERO(&allfd);
	FD_SET(sockfd, &allfd);
	maxFd = sockfd;

	// assign IP, PORT 
	servaddr.sin_family = AF_INET; 
	servaddr.sin_addr.s_addr = htonl(2130706433); //127.0.0.1
	servaddr.sin_port = htons(atoi(argv[1])); 
  
	// Binding newly created socket to given IP and verification 
	if (((bind(sockfd, (const struct sockaddr *)&servaddr, sizeof(servaddr))) < 0) || 0 > listen(sockfd, 10))
		error(NULL);
	len = sizeof(cli);
	char msg[1000];
	char buffer[1000];
	while (1)
	{
		bzero(msg, strlen(msg));
		bzero(buffer, strlen(buffer));
		readfd = writefd = allfd;
		if (0 > select(maxFd + 1, &readfd, &writefd, 0, 0))
			continue ;
		for (int i = 0; i <= maxFd; i++)
		{
			// VER si esta en lectura
			if (FD_ISSET(i, &readfd))
			{
				//Lectura : ver si es socket
				if (i == sockfd)
				{
					//Si es socket aceptar cliente
					connfd = accept(sockfd, (struct sockaddr *)&cli, &len);
					if (0 > connfd)
						continue ;
					FD_SET(connfd, &allfd);
					if (maxFd < connfd)
						maxFd = connfd;
					clients[connfd].id = maxClient++;	
					sprintf(msg, "server: client %d just arrived\n", clients[connfd].id);
					sendMessage(maxFd, writefd, connfd, msg);
				}
				else
				{
					printf("Entra elseahahahahahahah\n");
					if (0 >= recv(i, buffer, 1000, 0))
					{
					//si recv falla o cliente desconecta  gestionar desconexion de cliente
						sprintf(msg, "server: client %d just left\n", clients[i].id);
						sendMessage(maxFd, writefd, i, msg);
						FD_CLR(i, &allfd);
						if(clients[i].msg)
							free(clients[i].msg);	
						clients[i].msg = NULL;
						clients[i].id = -1;
						close(i);
					}
					else
					{
						printf("In progress read message, concat with old and print only first line\n");
					//si recv no falla unir con leido previo y sacar solo primera linea resto guardar para futuros recv
					}
				}
			}
			//Escritura: entonces volver a revisar fds y sus estados (acabar buble for y continuear en while)
			break;
					
		}
	}
/*
	connfd = accept(sockfd, (struct sockaddr *)&cli, &len);
	if (connfd < 0) { 
        printf("server acccept failed...\n"); 
        exit(0); 
    } 
    else
        printf("server acccept the client...\n");
*/
}
