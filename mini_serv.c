#include <unistd.h>
#include <sys/types.h> // libreria con los tipos de dominio de sockets
#include <sys/socket.h> // sockets
#include <netinet/in.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#define BUFFLEN 10000

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

int main(int argc, char **argv)
{
	int fds, clients;
	atexit(leaks);
	if (argc != 2)
	{
		printError("Wrong number of arguments");
		exit(1);
	}
	//Abrir socket protocolo itcp4
	int fd_socket = socket(AF_INET, SOCK_STREAM, 0);
	if (fd_socket < 0)
		printError(NULL);

	fd_set setRead, setWrite, setStatus;
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
	int lencli = sizeof(cliaddr);
	char buffer[BUFFLEN];
	clients = 0;

	while (1)
	{
		setRead = setWrite = setStatus;	
		if (0 > select(fds + 1, &setRead, &setWrite, 0, 0))
			continue ;
			//printError(NULL);
		for (int id = 0; id <= fds; id++)
		{
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
					write(1, "Conectado\n", 10);
					for (int i = 0; i < fds; i++)
					{
						if (i != fd_client && FD_ISSET(i, &setWrite))
						{
							if (0 > send(i, buffer, strlen(buffer), 0))
								printError(NULL);
						}
					}
					break ;
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
						//Recortar mensaje por líneas
						//Añadir mensaje al principio dela línea y enviar
//						repetir hasta que no queden líneas
/*
						int start, end;
						start = end = 0;
						do {
							sprintf(buffer, "client: %d: ", clients[id].id);
							while (client[id].msg[end] != '\n')
								end++;
							
							sprintf(client[id].msg, "client: %d: %s\n", clients[id].id,
						} while (start < strlen(buffer));
*/
						for (int i = 0; i < fds; i++)
						{
							if (i != id && FD_ISSET(i, &setWrite))
							{
								if (0 > send(i, buffer, strlen(buffer), 0))
									printError(NULL);
							}
						}
						write(1, buffer, msg_len);
					}
					else
					{
						sprintf(buffer, "server: client %d just left\n", client[id].id);
						for (int i = 0; i < fds; i++)
						{
							if (i != id && FD_ISSET(i, &setWrite))
							{
								if (0 > send(i, buffer, strlen(buffer), 0))
									printError(NULL);
							}
						}
						write(1, "Desconectado\n", 13);
						FD_CLR(id, &setStatus);
						close(id);
					}
				}
			}
		}
	}

	return (0);
}

