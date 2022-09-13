#include <fcntl.h>
#include "../include/Server.hpp"

template <class T1, class T2>
void deleteMap(std::map<T1, T2> &map){
	typename std::map<T1, T2>::iterator it1 = map.begin();
	typename std::map<T1, T2>::iterator it2 = it1;

	while (it1 != map.end())
	{
		it1++;
		delete it2->second;
		it2 = it1;
	}
};

Server::Server(int port, std::string password) : _command(this)
{
	_pollLet = 0;
	_maxClient = 0;
	_port = port;
	_password = password;

	//generate serversocket
	sock_init();
	_pollClient[0].fd = _serverSocketFd;
	_pollClient[0].events = POLLIN;
	for (int i = 1; i < OPEN_MAX; i++)
		_pollClient[i].fd = -1;
}

Server::~Server(){
	std::cout << "server destructer called\n";
	std::map<int, Client *>::iterator it = _clientList.begin();	
	close(_serverSocketFd);
	for(; it != _clientList.end(); it++){
		close(it->first);
	}
	deleteMap(_clientList);
	deleteMap(_channelList);
}

int	Server::pollingEvent()
{
	_clientLen = sizeof(_clientAddr);
	//generate socket for data recv
	_clientFd = accept(_serverSocketFd, (struct sockaddr *)&_clientAddr, &_clientLen);
	if (_clientFd < 0)
	{
		std::cerr << "Error: fail to accept client" << std::endl;
		exit (1);
	}
	int fcntlRet = fcntl(_clientFd, F_SETFL, O_NONBLOCK);
	if (fcntlRet == -1)
	{
		std::cerr << "Error: fail to fcntl" << std::endl;
		exit (1);
	}

	_clientList.insert(std::pair<int, Client *>(_clientFd, new Client(_clientFd)));
	std::cout << "Accept Client fd: " << _clientList.find(_clientFd)->first << "*" << std::endl;
	
	int index;
	for (index = 1; index < OPEN_MAX; index++)
	{
		if (_pollClient[index].fd < 0)
		{
			_pollClient[index].fd = _clientFd;
			break;
		}
	}
	_pollClient[index].events = POLLIN;
	if (index > _maxClient)
		_maxClient = index;
	if (--_pollLet <= 0)
		return (-1);
	return (0);
}

int	Server::sock_init()
{
	_serverSocketFd = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);

	if (_serverSocketFd == -1)
	{
		std::cerr << "Error: fail to socket" << std::endl;
		exit(1);
	}
	_serverSocketAddr.sin_family = AF_INET;
	_serverSocketAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	_serverSocketAddr.sin_port = htons(_port);
	for (int i = 0; i < 8; i++)
		_serverSocketAddr.sin_zero[i] = 0;

	int			optval = true;
	socklen_t	optlen = sizeof(optval);
	setsockopt(_serverSocketFd, SOL_SOCKET, SO_REUSEADDR, (void *)&optval, optlen);
	
	if (bind(_serverSocketFd, (const sockaddr *)&_serverSocketAddr, sizeof(_serverSocketAddr)) == -1)
	{
		std::cerr << "Error: fail to bind" << std::endl;
		exit (1);
	}
	if (listen(_serverSocketFd, 15) == -1)
	{
		std::cerr << "Error: fail to listen" << std::endl;
		exit (1);
	}
	std::cout << "listening" << std::endl;
	return (0);
}

void	Server::relayEvent()
{
	char	buf[512];
	for (int i = 1; i <= _maxClient; i++)
	{
		if (_pollClient[i].fd < 0)
			continue;
		if (_pollClient[i].revents == POLLIN)
		{
			memset(buf, 0x00, 512);
			//recv data
			if (recv(_pollClient[i].fd, buf, 512, 0) == -1)
			{
				std::vector<std::string> tmp_vec;
				tmp_vec.push_back("QUIT");
				tmp_vec.push_back(":lost connection");
				_command.quit(tmp_vec, findClient(_pollClient[i].fd));
				_pollClient[i].fd = -1;
			}
			else
			{
				Client	*targetClient = (_clientList.find(_pollClient[i].fd))->second;
				targetClient->appendRecvBuffer(std::string(buf));
				
				std::cout << "\n---- recvMsgBuf ----" << std::endl;
				std::cout << "[" << targetClient->getRecvBuffer() << "]" << std::endl;
				std::cout << "pollfd: " << _pollClient[i].fd << std::endl;
				std::cout << "--- endRecvMsgBuf ---" << std::endl;
				
				if (targetClient->getRecvBuffer().find("\r\n") == std::string::npos)
					continue;
				std::vector<std::string> cmd = split(targetClient->getRecvBuffer(), "\r\n");
				if (cmd[0] == "")
					continue;
				//Print Msg(Command) from Client
				print_stringVector(cmd);

				if (!(targetClient->getRegist() & REGI))
					_command.welcome(cmd, (_clientList.find(_pollClient[i].fd))->second, _clientList);
				else
				{
					std::vector<std::string>::iterator cmd_it = cmd.begin();
					while (cmd_it != cmd.end())
					{
						std::vector<std::string> result = split(*cmd_it, " ");
						check_cmd(result, targetClient);
						cmd_it++;
					}
					targetClient->getRecvBuffer().clear();
				}
			}
		}
		else if (_pollClient[i].revents & POLLERR)
		{
			std::cerr << "Error: fail to poll" << std::endl;
			exit(1);
		}
		else if (_pollClient[i].revents & POLLHUP)
		{
			std::vector<std::string> tmp_vec;
			tmp_vec.push_back("QUIT");
			tmp_vec.push_back(":lost connection");
			_command.quit(tmp_vec, findClient(_pollClient[i].fd));
			_pollClient[i].fd = -1;
		}
	}
	//Send Msg to Client.
	std::map<int, Client *>::iterator it = _clientList.begin();
	for (; it != _clientList.end(); it++)
	{
		if (it->second->getMsgBuffer().empty() == false)
		{
			std::string str = it->second->getMsgBuffer();
			send(it->first, str.c_str(), str.length(), 0);
			std::cout << "sendMsgs to fd: <" << it->first << ">" << std::endl;
			std::cout << str << std::endl;
			str.clear();
			it->second->clearMsgBuffer();
		}
	}
}

void	Server::check_cmd(std::vector<std::string> cmd_vec, Client *client)
{
	if (cmd_vec[0] == "NICK")
		_command.nick(cmd_vec, client);
	else if (cmd_vec[0] == "JOIN")
		_command.join(cmd_vec, client);
	else if (cmd_vec[0] == "KICK")
		_command.kick(cmd_vec, client);
	else if (cmd_vec[0] == "PRIVMSG")
		_command.privmsg(cmd_vec, client);
	else if (cmd_vec[0] == "NOTICE")
		_command.notice(cmd_vec, client);
	else if (cmd_vec[0] == "PING")
		_command.pong(cmd_vec, client);
	else if (cmd_vec[0] == "PART")
		_command.part(cmd_vec, client);
	else if (cmd_vec[0] == "QUIT")
		_command.quit(cmd_vec, client);
	else if (cmd_vec[0] == "PASS" || cmd_vec[0] == "USER")
		_command.alreadyRegist(client);
	else if (cmd_vec[0] == "MODE")
		_command.oper(cmd_vec, client);
	else
		std::cout << cmd_vec[0] << ": undefined cmd\n\n";
}

std::map<int, Client *>	&Server::getClientList()
{
	return _clientList;
}

std::map<std::string, Channel *> &Server::getChannelList()
{
	return _channelList;
}

std::string	Server::getPass()
{
	return _password;
}

Client	*Server::findClient(int fd)
{
	std::map<int, Client *>::iterator it;
	if ((it = _clientList.find(fd)) != _clientList.end())
		return (it->second);
	return NULL;
}

Client	*Server::findClient(std::string nick)
{
	std::map<int, Client *>::iterator it = _clientList.begin();
	for (; it != _clientList.end(); it++)
	{
		if (it->second->getNickName() == nick)
			return (it->second);
	}
	return NULL;
}

Channel* Server::findChannel(std::string name) {
	if (_channelList.find(name) == _channelList.end())
		return NULL;
	return _channelList.find(name)->second;
}

void	Server::addChannelList(std::string channelName, int fd)
{
	_channelList.insert(std::pair<std::string, Channel *>(channelName, new Channel(channelName, fd)));
}

void	Server::removeUnconnectClient(int fd)
{
	Client	*targetClient = findClient(fd);

	std::string str = targetClient->getMsgBuffer();
	send(fd, str.c_str(), str.length(), 0);

	std::cout << "remove client Msg to <" << fd << ">" << std::endl;
	std::cout << str << std::endl;
	str.clear();
	targetClient->clearMsgBuffer();
	getClientList().erase(fd);
	close(fd);
	delete targetClient;
}

int		Server::execute()
{
	while (1)
	{
		_pollLet = poll(_pollClient, _maxClient + 1, -1);
		if (_pollLet == 0 || _pollLet == -1)
		{
			std::cerr << "Error: fail to poll" << std::endl;
			break ;
		}
		if (_pollClient[0].revents & POLLIN)
		{
			if (pollingEvent() == -1)
				continue;
		}
		relayEvent();
	}
	return (0);
}

