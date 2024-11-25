#include <unistd.h>
#include <sys/types.h> // libreria con los tipos de dominio de sockets
#include <sys/socket.h> // sockets
#include <netinet/in.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <limits.h>

#define BUFFLEN 1000

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
typedef struct s_client
{
	int	id;
	char	msg[BUFFLEN];
}	t_client;

t_client client[1024];
int fds, clients;
fd_set setRead, setWrite, setStatus;
char line[BUFFLEN + 100];
char buffer[BUFFLEN];

void sendMsg(const int sender, const char *msg)
{
	for (int i = 0; i <= fds; i++)
	{
		if (i != sender && FD_ISSET(i, &setWrite))
			if (0 > send(i, msg, strlen(msg), 0))
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
	if (0 > listen(fd_socket, 100))
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
		for (int id = 0; id <= fds; id++)
		{
			bzero(buffer, sizeof(buffer));
			bzero(line, sizeof(line));
			bzero(client[id].msg, sizeof(client[id].msg));
			//Comprobar Si el fd esta en lectura o escritura
			if (FD_ISSET(id, &setRead))
			{
				if (id == fd_socket)
				{
					//Aceptar conexión ya que le socket del serv solo escucha nuevas peticiones
					//Aceptamos la primera conexión de un cliente y creamos su respectivo socket ...
					int fd_client = accept(fd_socket, (struct sockaddr *) &cliaddr, &lencli);
					if (0 > fd_client)
						continue ;
					//	printError(NULL);
					//Actualizar max fd con el nuevo creado (cuando se desconecten revisar si hay más que antes o no
					if (fds < fd_client)
						fds = fd_client;
					//Añadir nuevo fd al select
					FD_SET(fd_client, &setStatus);
					client[fd_client].id = clients++;
					sprintf(buffer, "server: client %d just arrived\n", client[fd_client].id);
					//Cambiar por mensaje a todos los clientes
		//			write(1, "Conectado\n", 10);
					sendMsg(fd_client, buffer);
					bzero(buffer, strlen(buffer));
				}
				else
				{
					//Algún cliente evento de lectura por parte del serv
					//Server espera bloqueante un mensaje del fd_client que le digamos en este caso el único cliente que se puede conectar
					int msg_len = recv(id, &buffer, BUFFLEN, 0);
					//if msg_len < 0 handler client desconection
					//Envia mensaje en el buffer al resto de clientes conectados
					if (msg_len > 0)
					{
						char *aux, *endLine;
						aux = endLine = buffer;
						do
						{
							while (endLine && *endLine != '\n')
								endLine++;
							if (endLine)
							{
								*endLine = '\0';
								if (!strcpy(client[id].msg, aux))
									printError(NULL);
							}
							if (client[id].msg)
								sprintf(line, "client %d: %s\n", client[id].id, client[id].msg);
							else
								sprintf(line, "client %d: %s\n", client[id].id, aux);
							sendMsg(id, line);
							if (((endLine - buffer) + 1) < msg_len)
								aux = ++endLine;
							bzero(client[id].msg, strlen(client[id].msg));
							bzero(line, strlen(line));
						} while (endLine);
//						sendMsg(id, buffer);
//						bzero(buffer, strlen(buffer));
		//				write(1, "mensaje enviado\n", 16);
					}
					else
					{
						sprintf(buffer, "server: client %d just left\n", client[id].id);
						sendMsg(id, buffer);
		//				write(1, "Desconectado\n", 13);
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

