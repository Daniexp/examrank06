#include <unistd.h>
#include <sys/types.h> // libreria con los tipos de dominio de sockets
#include <sys/socket.h> // sockets
#include <netinet/in.h>
#include <string.h>
#include <stdlib.h>

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
}

int main(int argc, char **argv)
{
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

	//Aceptamos la primera conexión de un cliente y creamos su respectivo socket ...
	struct sockaddr_in cliaddr;
	int lencli = sizeof(cliaddr);

	int fd_client = accept(fd_socket, (struct sockaddr *) &cliaddr, &lencli);
	if (0 > fd_client)
		printError(NULL);

	//Server espera bloqueante un mensaje del fd_client que le digamos en este caso el único cliente que se puede conectar
	char buffer[10000];
	int msg_len = recv(fd_client, &buffer, 10000, 0);

	write(1, buffer, msg_len);

	return (0);
}

