#ifndef COMMAND_HPP
#define COMMAND_HPP

#include <unistd.h>
#include <vector>
#include <map>
#include <set>
#include <string>
#include <iostream>
#include <sstream>

#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/poll.h>

#include "Channel.hpp"
#include "Client.hpp"
#include "Util.hpp"
#include "Define.hpp"

class Server;

class Command
{
	private :
		Server *_server;
		bool	isLetter(char c);
		bool	isNumber(char c);
		bool	isSpecial(char c);
		bool	isDuplication(char c);
		bool	nickValidate(std::string s);

		void	channelMessage(std::string msg, Client *client, Chnnel *channel);
		std::string makeFullname(int fd);
		void welcomeMsg(int fd, std::string flag, std::string msg, std::string name);
		void allInChannelMsg(int target, std::string channelName, std::string command, std::string msg);
		void nameListMsg(int fd, std::string channelName);
		void makeNumericReply(int fd, std::string flag, std::string str);
		void makeCommandReply(int fd, std::string command, std::string str);

	public :
		Command(Server *server);
		~Command();
		void	pong(std::vector<std::string> s, Client *client);
		void	nick(std::vector<std::string> s, Client *client);
		void	join(std::vector<std::string> s, Client *client);
		void	kick(std::vector<std::string> s, Client *client);
		void	privmsg(std::vector<std::string> s, Client *client);
		void	notice(std::vector<std::string> s, Client *client);
		void	part(std::vector<std::stinrg> s, Client *client);
		void	quit(std::vector<std::string> s, Client *client);
		void welcome(std::vector<std::string> cmd, Client *client, std::map<int, Client *> clientList);
		void alreadyRegist(Client *client);
};

#endif
