#include "../include/Server.hpp"
#include "../include/Command.hpp"
#include "../include/Define.hpp"

Command::Command(Server *server)
{
	_server = server;
}

Command::~Command()
{
}

bool		Command::isLetter(char c)
{
	if (('a' <= c && c <= 'z') || ('A' <= c && c <= 'Z'))
		return true;
	else
		return false;
}

bool		Command::isNumber(char c)
{
	if ('0' <= c && c <= '9')
	    return true;
	else
	    return false;
}

bool		Command::isSpecial(char c)
{
	if (c == '-' || c == '[' || c == ']' || c == '\\' || c == '`' || c == '^' || c =='{' || c == '}')
		return true;
	else
		return false;
}

bool		Command::isDuplication(std::string s, std::map<int, Client *> clientList)
{
	std::map<int, Client *>::iterator it = clientList.begin();
	while (it != clientList.end())
	{
		if (it->second->getNickName() == s)
			return true;
		it++;
	}
	return false;
}

bool		Command::nickValidate(std::string s)
{
	if (0 >= s.length() || s.length() > 9)
		return false;
	if (isLetter(s[0]) == false)
		return false;
	for (int i = 1; i < (int)(s.length()); i++)
	{
		if (!isLetter(s[i]) && !isSpecial(s[i]) && !isNumber(s[i]))
			return false;
	}
	return true;
}

void		Command::pong(std::vector<std::string> s, Client *client)
{
    client->appendMsgBuffer("PONG" + s[1] + "\r\n");
}

void	Command::nick(std::vector<std::string> s, Client *client)
{
	if (s.size() < 2)
	{
		makeNumericReply(client->getClientFd(), ERR_NEEDMOREPARAMS, "NICK: Not enough parameters");
		return ;
	}
	if (isDuplication(s[1], _server->getClientList()))
	{
		makeNumericReply(client->getClientFd(), ERR_NICKNAMEINUSE, s[1] + " :Nickname is already in use");
		return ;
	}
	if (!nickValidate(s[1]))
	{
		makeNumericReply(client->getClientFd(), ERR_ERRONEUSNICKNAME, s[1] + " :Erroneus nickname");
		return ;
	}

	std::vector<std::string> participatingChannelName = client->getMyChannelList();
	makeCommandReply(client->getClientFd(), "NICK", s[1]);
	if (participatingChannelName.size() != 0)
	{
		std::vector<std::string>::iterator participatingChannelNameIt = participatingChannelName.begin();
		std::set<int> fdList;
		while (participatingChannelNameIt != participatingChannelName.end())
		{
			Channel *channel = _server->findChannel(*participatingChannelNameIt);
			if (channel != NULL)
			{
				std::vector<int> fdsInChannel = channel->getMyClientFdList();
				std::vector<int>::iterator fdsInChannelIt = fdsInChannel.begin();
				while (fdsInChannelIt != fdsInChannel.end())
				{
					if (client->getClientFd() != (*fdsInChannelIt))
						fdList.insert(*fdsInChannelIt);
					fdsInChannelIt++;
				}
			}
			participatingChannelNameIt++;
		}
		std::set<int>::iterator fdsIt = fdList.begin();
		while (fdsIt != fdList.end())
		{
			Client *recvClient = _server->findClient(*fdsIt);
			recvClient->appendMsgBuffer(":" + client->getNickName() + " NICK" + s[1] + "\r\n");
			fdsIt++;
		}
		fdList.clear();
	}
	client->setNickName(s[1]);
}

void		Command::join(std::vector<std::string> s, Client *client)
{
	if (s.size() < 2)
	{
		makeNumericReply(client->getClientFd(), ERR_NEEDMOREPARAMS, " Not enough parameters");
		return ;
	}
	std::vector<std::string> joinChannel = split(s[1], ",");
	std::vector<std::string>::iterator joinChannelIt = joinChannel.begin();
	while (joinChannelIt != joinChannel.end())
	{
		std::map<std::string, Channel *>::iterator findChannelIt = _server->getChannelList().find(*joinChannelIt);
		if (findChannelIt != _server->getChannelList().end())
		{
			std::string channelName = (*findChannelIt).second->getChannelName();
			(*findChannelIt).second->addMyClientList(client->getClientFd());
			client->addChannelList(channelName);
			allInChannelMsg(client->getClientFd(), channelName, "JOIN", "");
		}
		else
		{
			_server->addChannelList(*joinChannelIt, client->getClientFd());
			_server->findChannel(*joinChannelIt)->addMyClientList(client->getClientFd());
			client->addChannelList(*joinChannelIt);
			allInChannelMsg(client->getClientFd(), *joinChannelIt, "JOIN", "");
		}
		nameListMsg(client->getClientFd(), *joinChannelIt);
		joinChannelIt++;
	}
}

void	Command::kick(std::vector<std::string> s, Client *client)
{
	int sLength = s.size();
	if (sLength < 3)
	{
		makeNumericReply(client->getClientFd(), ERR_NEEDMOREPARAMS, "KICK :Not enough parameters");
		return ;
	}
	std::vector<std::string> channelNames = split(s[1], ",");
	std::vector<std::string>::iterator channelNameIt = channelNames.begin();
	while (channelNameIt != channelNames.end())
	{
		Channel *channel = _server->findChannel(*channelNameIt);
		if (channel == NULL)
			makeNumericReply(client->getClientFd(), ERR_NOSUCHCHANNEL, *channelNameIt + " :No such channel");
		else
		{
			std::vector<std::string> kickedUserNickName = split(s[2], ",");
			std::vector<std::string>::iterator kickedUserNickNameIt = kickedUserNickName.begin();
			Client *kickedClient;
			while (kickedUserNickNameIt != kickedUserNickName.end())
			{
				kickedClient = _server->findClient(*kickedUserNickNameIt);
				if (kickedClient == NULL)
					makeNumericReply(client->getClientFd(), "401", *kickedUserNickNameIt + " :No such nick/channel");
				else
				{
					std::vector<int> operatorFd = channel->getMyOperator();
					if (find(operatorFd.begin(), operatorFd.end(), client->getClientFd()) == operatorFd.end())

						makeNumericReply(client->getClientFd(), ERR_CHANOPRIVSNEEDED, *channelNameIt + " :You're not channel operator");
					else if (!channel->checkClientInChannel(kickedClient->getClientFd()))
						makeNumericReply(client->getClientFd(), ERR_USERNOTINCHANNEL, *kickedUserNickNameIt + " " + *channelNameIt + " :They aren't on that channel");
					else
					{
						if (sLength > 3)
							allInChannelMsg(client->getClientFd(), *channelNameIt, "KICK", *kickedUserNickNameIt + " " + appendStringColon(3, s));
						else
							allInChannelMsg(client->getClientFd(), *channelNameIt, "KICK", *kickedUserNickNameIt);
						channel->removeClientList(kickedClient->getClientFd());
						kickedClient->removeChannel(*channelNameIt);
						if (channel->getMyClientFdList().empty() == true)
						{
							_server->getChannelList().erase(channel->getChannelName());
							delete channel;
						}
						else if (channel->getMyOperator().empty() == true)
							channel->setMyOperator(*(channel->getMyClientFdList().begin()));
					}
				}
				kickedUserNickNameIt++;
			}
		}
		channelNameIt++;
	}
}

void	Command::privmsg(std::vector<std::string> s, Client *client)
{
	if (s.size() <= 2)
	{
		makeNumericReply(client->getClientFd(), ERR_NEEDMOREPARAMS, "PRIVMSG :Not enough parameters");
		return ;
	}
	std::vector<std::string> target = split(s[1], ",");
	std::vector<std::string>::iterator targetNameIt = target.begin();
	while (targetNameIt != target.end())
	{
		if ((*targetNameIt)[0] == '#')
		{
			if (_server->findChannel(*targetNameIt) == NULL)
				makeNumericReply(client->getClientFd(), ERR_NOSUCHCHANNEL, *targetNameIt + " :No such channel");
			else
				channelMessage(appendStringColon(2, s), client, _server->findChannel(*targetNameIt));
		}
		else
		{
			if (_server->findClient(*targetNameIt) == NULL)
				makeNumericReply(client->getClientFd(), ERR_NOSUCHNICK, *targetNameIt + " :No such nick/channel");
			else
			{
				Client *receiver = _server->findClient(*targetNameIt);
				if (receiver != NULL)
					makePrivMessage(_server->findClient(*targetNameIt), client->getNickName(), receiver->getNickName(), appendStringColon(2, s));
			}
		}
		targetNameIt++;
	}
}

void	Command::notice(std::vector<std::string> s, Client *client)
{
	Client *receiver = _server->findClient(s[1]);
	if (receiver != NULL)
		receiver->appendMsgBuffer(makeFullname(client->getClientFd()) + " NOTICE " + receiver->getNickName() + " " + appendStringColon(2, s) + "\r\n");
}

void	Command::makePrivMessage(Client *client, std::string senderName, std::string receiver, std::string msg)
{
	if (client == NULL)
		return ;
	client->appendMsgBuffer(":" + senderName + " PRIVMSG " + receiver + " " + msg + "\r\n");
}

void	Command::channelMessage(std::string msg, Client *client, Channel *channel)
{
	std::vector<int> clientInChannel = channel->getMyClientFdList();
	std::vector<int>::iterator clientInChannelIt = clientInChannel.begin();
	while (clientInChannelIt != clientInChannel.end())
	{
		if (client->getClientFd() != *clientInChannelIt)
			makePrivMessage(_server->findClient(*clientInChannelIt), client->getNickName(),
						channel->getChannelName(), msg);
		clientInChannelIt++;
	}
}

void	Command::part(std::vector<std::string> s, Client *client)
{
	if (s.size() < 2)
	{
		makeNumericReply(client->getClientFd(), ERR_NEEDMOREPARAMS, " :Not enough paramerters");
		return ;
	}
	std::vector<std::string> partChannel = split(s[1], ",");
	std::vector<std::string>::iterator partChannelIt = partChannel.begin();
	while (partChannelIt != partChannel.end())
	{
		std::vector<std::string>::iterator searchChannelNameIt = client->findMyChannelIt(*partChannelIt);
		if (searchChannelNameIt != client->getMyChannelList().end())
		{
			allInChannelMsg(client->getClientFd(), *searchChannelNameIt, "PART", appendStringColon(2, s));
			Channel *targetChannel = _server->findChannel(*partChannelIt);
			targetChannel->removeClientList(client->getClientFd());
			client->removeChannelList(searchChannelNameIt);
			if (targetChannel->getMyClientFdList().empty() == true)
			{
				_server->getChannelList().erase(targetChannel->getChannelName());
				delete targetChannel;
			}
			else if (targetChannel->getMyOperator().empty() == true)
				targetChannel->setMyOperator(*(targetChannel->getMyClientFdList().begin()));
		}
		else
		{
			if (_server->getChannelList().find(*partChannelIt) == _server->getChannelList().end())
				makeNumericReply(client->getClientFd(), ERR_NOSUCHCHANNEL, *partChannelIt + " :No such channel");
			else
				makeNumericReply(client->getClientFd(), ERR_NOTONCHANNEL, *partChannelIt + " :You're not on that channel");
		}
		partChannelIt++;
	}
}

void Command::quit(std::vector<std::string> s, Client *client)
{
	std::vector<std::string>::iterator channelListInClientClassIt = client->getMyChannelList().begin();
 	while (channelListInClientClassIt != client->getMyChannelList().end())
    {
        Channel *targetChannel = _server->findChannel(*channelListInClientClassIt);
        targetChannel->removeClientList(client->getClientFd());
		allInChannelMsg(client->getClientFd(), targetChannel->getChannelName(), "PART", appendStringColon(1,s));
        if (targetChannel->getMyClientFdList().empty() == true)
        {
            _server->getChannelList().erase(targetChannel->getChannelName());
            delete targetChannel;
        }
		else if (targetChannel->getMyOperator().empty() == true)
				targetChannel->setMyOperator(*(targetChannel->getMyClientFdList().begin()));
        channelListInClientClassIt++;
    }
    _server->getClientList().erase(client->getClientFd());
    close(client->getClientFd());
    delete client;
}

void	Command::oper(std::vector<std::string> s, Client *client)
{
	int sLength = s.size();
	if (sLength > 2 && s[2] != "+o")
	{
		std::cout << s[0] << ": undefined cmd\n\n";
		return;
	}
	if (sLength < 4)
	{
		makeNumericReply(client->getClientFd(), ERR_NEEDMOREPARAMS, "OP :Not enough parameters");
		return;
	}

	std::vector<std::string> channelNames = split(s[1], ",");
	std::vector<std::string>::iterator channelNameIt = channelNames.begin();
	while (channelNameIt != channelNames.end())
	{
		Channel *channel = _server->findChannel(*channelNameIt);
		if (channel == NULL)
			makeNumericReply(client->getClientFd(), ERR_NOSUCHCHANNEL, *channelNameIt + " :No such channel");
		else
		{
			std::vector<std::string> addedUserNickName = split(s[3], ",");
			std::vector<std::string>::iterator addedUserNickNameIt = addedUserNickName.begin();	
			Client *addedClient;
			std::vector<int> operatorFd = channel->getMyOperator();
			if (find(operatorFd.begin(), operatorFd.end(), client->getClientFd()) == operatorFd.end())
			{
				makeNumericReply(client->getClientFd(), ERR_CHANOPRIVSNEEDED, *channelNameIt + " :You're not channel operator");
				channelNameIt++;
				continue ;
			}
			while (addedUserNickNameIt != addedUserNickName.end())
			{
				addedClient = _server->findClient(*addedUserNickNameIt);
				if (addedClient == NULL)
					makeNumericReply(client->getClientFd(), "401", *addedUserNickNameIt + " :No such nick/channel");
				else
				{
					if (!channel->checkClientInChannel(addedClient->getClientFd()))
						makeNumericReply(client->getClientFd(), ERR_USERNOTINCHANNEL, *addedUserNickNameIt + " " + *channelNameIt + " :They aren't on that channel");
					else
					{
						if (sLength > 4)
							allInChannelMsg(client->getClientFd(), *channelNameIt, "MODE", *addedUserNickNameIt + " added to operators." + " " + appendStringColon(4, s));
						else
							allInChannelMsg(client->getClientFd(), *channelNameIt, "MODE", *addedUserNickNameIt + " added to operators.");
						channel->setMyOperator(addedClient->getClientFd());
					}
				}
				addedUserNickNameIt++;
			}
		}
		channelNameIt++;
	}
}

std::string Command::makeFullname(int fd)
{
	Client *targetClient = _server->findClient(fd);
	std::string test = (":" + targetClient->getNickName() + "!" + targetClient->getUserName() + "@" + targetClient->getServerName());
	return (test);

};

void Command::welcomeMsg(int fd, std::string flag, std::string msg, std::string name)
{
	Client *targetClient = _server->findClient(fd);
	targetClient->appendMsgBuffer(flag);
	targetClient->appendMsgBuffer(" ");
	targetClient->appendMsgBuffer(name);
	targetClient->appendMsgBuffer(" ");
	targetClient->appendMsgBuffer(msg);
	targetClient->appendMsgBuffer(" ");
	targetClient->appendMsgBuffer(name);
	targetClient->appendMsgBuffer("\r\n");
}

void Command::allInChannelMsg(int sendClient, std::string channelName, std::string command, std::string msg)
{
	Channel *channelPtr = _server->findChannel(channelName);
	std::vector<int> myClientList = channelPtr->getMyClientFdList();
	std::vector<int>::iterator myClientListIt = myClientList.begin();
	for(; myClientListIt < myClientList.end(); myClientListIt++)
	{
		Client *recvClient = _server->findClient(*myClientListIt);
		recvClient->appendMsgBuffer(makeFullname(sendClient) + " " + command + " " + channelName + " " + msg + "\r\n");
	}
}

void Command::nameListMsg(int fd, std::string channelName)
{
	Client *recvClient = _server->findClient(fd);
	recvClient->appendMsgBuffer(RPL_NAMREPLY);
	recvClient->appendMsgBuffer(" ");
	recvClient->appendMsgBuffer(recvClient->getNickName());
	recvClient->appendMsgBuffer(" = " + channelName);
	Channel *channelPtr = _server->findChannel(channelName);
	std::vector<int> clientList = channelPtr->getMyClientFdList();
	std::vector<int>::iterator clientListIt = clientList.begin();
	std::string name;
	recvClient->appendMsgBuffer(" :");

	std::vector<int> operList = channelPtr->getMyOperator();
	for (; clientListIt < clientList.end() - 1; clientListIt++)
	{
		if (find(operList.begin(), operList.end(), *clientListIt) != operList.end())
			recvClient->appendMsgBuffer("@");
		name = (_server->findClient(*clientListIt))->getNickName();
		recvClient->appendMsgBuffer(name);
		recvClient->appendMsgBuffer(" ");
	}
	if (find(operList.begin(), operList.end(), *clientListIt) != operList.end())
		recvClient->appendMsgBuffer("@");
	name = (_server->findClient(*clientListIt))->getNickName();
	recvClient->appendMsgBuffer(name);
	recvClient->appendMsgBuffer("\r\n");

	recvClient->appendMsgBuffer(RPL_ENDOFNAMES);
	recvClient->appendMsgBuffer(" " + recvClient->getNickName() +  " " + channelName + " :End of NAMES list" + "\r\n");
}



void Command::makeNumericReply(int fd, std::string flag, std::string str)
{
	Client *targetClient = _server->findClient(fd);
	targetClient->appendMsgBuffer(flag);
	targetClient->appendMsgBuffer(" ");
	targetClient->appendMsgBuffer(targetClient->getNickName());
	targetClient->appendMsgBuffer(" ");
	targetClient->appendMsgBuffer(str);
	targetClient->appendMsgBuffer("\r\n");
}

void Command::makeCommandReply(int fd, std::string command, std::string str)
{
	Client *targetClient = _server->findClient(fd);
	targetClient->appendMsgBuffer(":");
	targetClient->appendMsgBuffer(targetClient->getNickName());
	targetClient->appendMsgBuffer(" ");
	targetClient->appendMsgBuffer(command);
	targetClient->appendMsgBuffer(" ");
	targetClient->appendMsgBuffer(str);
	targetClient->appendMsgBuffer("\r\n");
}

void Command::welcome(std::vector<std::string> cmd, Client *client, std::map<int, Client *> clientList)
{
	std::vector<std::string>::iterator cmd_it = cmd.begin();
	while (cmd_it != cmd.end())
	{
		std::vector<std::string> result = split(*cmd_it, " ");
		if (result[0] == "PASS")
		{
			if (client->getRegist() & PASS)
			{
				makeNumericReply(client->getClientFd(), ERR_ALREADYREGISTRED, ":You are already checked Password");
				return ;
			}
			if (result.size() == 1)
			{
				makeNumericReply(client->getClientFd(), ERR_NEEDMOREPARAMS, ":Not enough parameters...\nYou are not Input Password");
				return ;
			}
			if (result[1] == _server->getPass())
			{
				client->setRegist(PASS);
			}
			else
            {
				makeNumericReply(client->getClientFd(), ERR_PASSWDMISMATCH, ":Server Password Incorrect");
				_server->removeUnconnectClient(client->getClientFd());
				return ;
            }
		}
		else if (client->getRegist() & PASS && result[0] == "NICK")
		{
			if (!nickValidate(result[1]))
			{
				makeNumericReply(client->getClientFd(), ERR_ERRONEUSNICKNAME, result[1] + " :Erroneus Nickname");
				return ;
			}
			while (isDuplication(result[1], clientList))
			{
				if (client->getNickName() == result[1])
					return;
				std::string dup = result[1];
				result[1].append("_");
			}
			client->setNickName(result[1]);
			client->setRegist(NICK);
			makeCommandReply(client->getClientFd(), "NICK", result[1]);
		}
		else if (client->getRegist() & PASS && client->getRegist() & NICK && result[0] == "USER")
		{
			if (result.size() < 5)
			{
				makeNumericReply(client->getClientFd(), ERR_NEEDMOREPARAMS, "USER :Not enough parameters...\n/USER <username> <hostname> <servername> <:realname>");
				return ;
			}
			client->setUser(result[1], result[2], result[3], appendStringColon(4, result));
			client->setRegist(USER);
		}
		else if (result[0] != "CAP")
		{
			if (!(client->getRegist() & PASS))
			{
				makeNumericReply(client->getClientFd(), ERR_NOTREGISTERED, ":You have not registered Server's Password");
				_server->removeUnconnectClient(client->getClientFd());
			}
			else if (!(client->getRegist() & NICK))
				makeNumericReply(client->getClientFd(), ERR_NOTREGISTERED, ":You have not registered Nickname");
			else if (!(client->getRegist() & USER))
				makeNumericReply(client->getClientFd(), ERR_NOTREGISTERED, ":You have not registered USER info");
			return ;
		}
		cmd_it++;
	}
	if (client->getRegist() & PASS && client->getRegist() & NICK && client->getRegist() & USER)
    {
		welcomeMsg(client->getClientFd(), RPL_WELCOME, ":Welcome to the Internet Relay Network", client->getNickName());
		client->setRegist(REGI);
    }
}

void	Command::alreadyRegist(Client *client)
{
	makeNumericReply(client->getClientFd(), ERR_ALREADYREGISTRED, ":You are already registed");
}
