#include <iostream>
#include <cstdlib>
#include <signal.h>
#include "../include/Server.hpp"

Server *server;

void	sigIntHandler(int num)
{
	if (num == SIGINT)
		delete server;
	exit(1);
}

void	sigQuitHandler(int num)
{
	if (num == SIGQUIT)
		delete server;
	exit(1);
}

static int checkPort(char *port)
{
	int i = 0;
	
	while (port[i])
	{
		if (port[i] < '0' || '9' < port[i])
		{
			std::cout << "Wrong Port Number\n";
			return (1);
		}
		i++;
	}
	return (0);
}

int		main(int argc, char **argv)
{
	if (argc != 3)
	{
		std::cout << "Wrong input: ./server <port> <passwrod>\n";
		return (1);
	}
	if (checkPort(argv[1]))
		return (1);
	signal(SIGINT, sigIntHandler);
	signal(SIGQUIT, sigQuitHandler);
	server = new Server(atoi(argv[1]), argv[2]);
	if (server->execute() < 0)
	{
		delete server;
		return (1);
	}
	delete server;
	return (0);
}
