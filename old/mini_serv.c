#include <unistd.h>
#include <sys/types.h> // libreria con los tipos de dominio de sockets
#include <sys/socket.h> // sockets
#include <netinet/in.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
#define BUFFLEN 100

void leaks(void)
{
	system("leaks -q a.out");
}

void printError(char *str)
{
	if (!str)
		str = "Fatal Error";
	write(2, str, strlen(str));
	write(2, "\n", 1);
	exit(1);
}

int client[1027];
int fds, clients;
fd_set setRead, setWrite, setStatus;

void sendMsg(const int sender, const char *msg)
{
	for (int i = 0; i <= fds; i++)
	{
		if (i != sender && FD_ISSET(i, &setWrite))
			if (0 > send(i, msg, strlen(msg), MSG_DONTWAIT))
				printError(NULL);
	}
}


int main(int argc, char **argv)
{
//	atexit(leaks);
	if (argc != 2)
	{
		printError("Wrong number of arguments");
		exit(1);
	}
	//Abrir socket protocolo itcp4
	int fd_socket = socket(AF_INET, SOCK_STREAM, 0);
	if (fd_socket < 0)
		printError(NULL);

	//Añadir fd al select
	fds = fd_socket;
	FD_ZERO(&setStatus);
	FD_SET(fd_socket, &setStatus);
	struct sockaddr_in servaddr;
	bzero(&servaddr, sizeof(servaddr));

	//Rellenar structura para el socket
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = htonl(2130706433); //127.0.0.1
	servaddr.sin_port = htons(atoi(argv[1]));

	//Unir struct a socket
	if (0 > bind(fd_socket, (struct sockaddr *) &servaddr, sizeof(servaddr)))
		printError(NULL);

	//Preparar socket para recibir nuevas conexiones maximo 100 antes de entrar en espera
	if (0 > listen(fd_socket, 10))
		printError(NULL);

	//Preparar conexión de un único cliente
	struct sockaddr_in cliaddr;
	unsigned int lencli = sizeof(cliaddr);
	clients = 0;

	while (1)
	{
		setRead = setWrite = setStatus;	
		if (0 > select(fds + 1, &setRead, &setWrite, 0, 0))
			continue ;
			//printError(NULL);
		for (int id = 2; id <= fds; id++)
		{
			char buffer[BUFFLEN];
			bzero(buffer, sizeof(buffer));
			if (FD_ISSET(id, &setRead))
			{
				if (id == fd_socket)
				{
					int fd_client = accept(fd_socket, (struct sockaddr *) &cliaddr, &lencli);
					if (0 > fd_client)
						continue ;
					//	break;
					if (fds < fd_client)
						fds = fd_client;
					FD_SET(fd_client, &setStatus);
					client[fd_client] = clients++;
					sprintf(buffer, "server: client %d just arrived\n", client[fd_client]);
					sendMsg(fd_client, buffer);
					bzero(buffer, strlen(buffer));
				}
				else
				{
					int msg_len = recv(id, &buffer, BUFFLEN, 0);
					if (msg_len > 0)
					{
						//Seguir leyendo hasta tener el mensaje completo
						int j = 0;
						for (int i = 0; j < msg_len - 1; i = ++j)
						{
							while (buffer[j] && buffer[j] != '\n')
								j++;
							if (buffer[j])
								buffer[j] = '\0';
							char msg[200];
							sprintf(msg, "client %d: %s\n", client[id], &(buffer[i]));
							sendMsg(id, msg);
							bzero(buffer, strlen(buffer));
							bzero(msg, strlen(msg));
						}
					}
					else
					{
						sprintf(buffer, "server: client %d just left\n", client[id]);
						sendMsg(id, buffer);
						FD_CLR(id, &setStatus);
						close(id);
						bzero(buffer, strlen(buffer));
					}
				}
				break;
			}
		}
	}

	return (0);
}

