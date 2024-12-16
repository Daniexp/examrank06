#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>

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

void printError(char *msg)
{
	if (!msg)
		msg = "Fatal error";
	write(1, msg, strlen(msg));
	write(1, "\n", 1);
	exit(1);
}
int sendMsg(int maxFd, fd_set *fd_write, int sender, char *msg)
{
	int send = 0;
	if (!fd_write || maxFd < 0 || sender < 0 || !msg)
		return -1;
	for (int i = 3; i <= maxFd; i++)
		if (sender != i && FD_ISSET(i, fd_write) && send(i, msg, strlen(msg), 0) >= 0)
			send++;
	return send;
}


int main(int argc, char **argv) {

	if (argc < 2)
		printError("Wrong number of arguments");
	int sockfd, connfd;
	unsigned int len;
	struct sockaddr_in servaddr, cli; 
	fd_set fd_write, fd_read, fd_list;

	// socket create and verification 
	sockfd = socket(AF_INET, SOCK_STREAM, 0); 
	if (sockfd == -1)
		printError(NULL);
	bzero(&servaddr, sizeof(servaddr)); 

	// assign IP, PORT 
	servaddr.sin_family = AF_INET; 
	servaddr.sin_addr.s_addr = htonl(2130706433); //127.0.0.1
	servaddr.sin_port = htons(atoi(argv[1])); 
  
	// Binding newly created socket to given IP and verification 
	if ((bind(sockfd, (const struct sockaddr *)&servaddr, sizeof(servaddr))) != 0) 
		printError(NULL);
	if (listen(sockfd, 10) != 0)
		printError(NULL);
/*
	len = sizeof(cli);
	connfd = accept(sockfd, (struct sockaddr *)&cli, &len);
	if (connfd < 0) { 
        printf("server acccept failed...\n"); 
        exit(0); 
    } 
    else
        printf("server acccept the client...\n");
*/
}
