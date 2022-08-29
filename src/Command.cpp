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
	if (particiipatingChannelName.size() != 0)
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
			Client *tmp = _server->findClient(*fdsIt);
			tmp->appendMsgBuffer(":" + client->getNickName() + " NICK" + s[1] + "\r\n");
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
	std::vector<std::stirng> joinChannel = split(s[1], ",");
	std::vector<std::string>::iterator it = joinChannel.begin();
	while (it != joinChannel.end())
	{
		std::map<std::string, Channel *>::iterator findChannelIt = _server->getChannelList().find(*it);
		if (findChannelIt != _server->getChannelList().end())
		{
			std::string channelName = (*findChannelIt).second->getChannelName();
			(*findChannelIt).second->addMyClientList(client->getClientFd());
			client->addChannelList(channelName);
			allInChannelMsg(client->getClientFd(), channelName, "JOIN", "");
		}
		else
		{
			_server->addChannelList(*it, client->getClientFd());
			_server->findChannel(*it)->addMyClientList(client->getClientFd());
			client->addChannelList(*it);
			allInChannelMsg(client->getClientFd(), *it, "JOIN", "");
		}
		nameListMsg(client->getClientFd(), *it);
		it++;
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
		channel *channel = _server->findChannel(*channelNameIt);
		if (channel == NULL)
			makeNumericReply(client->getClientFd(), ERR_NOSUCHCHANNEL, *channelNameIt + " :No such channel");
		else
		{
			std::vector<std::string> kickedUserNickName = split(s[2], ",");
			std::vector<std::string>::iterator kickedUserNickNameIt = kickedUserNickName.begin();
			Client *kickedClient;
			while (kickedUserNickNameIt != kickedUserNickName.end())
			{
				kickedClient = _server->findClient(*ickedUserNickNameIt);
				if (kickedClient == NULL)
					makeNumericReply(client->getClientFd(), "401", *kickedUserNickNameIt + " :No such nick/channel");
				else
				{
					int operatorFd = channel->getMyOperator();
					if (operatorFd != client->getClientFd())
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
						else
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
