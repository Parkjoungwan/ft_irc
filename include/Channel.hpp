#ifndef	CHANNEL_HPP
#define CHANNEL_HPP

#include <vector>
#include <string>
#include "Client.hpp"

class Client;

class Channel
{
	private:
		int			_operator;
		std::string		_channelName;
		std::vector<int>	_myClientFdList;

	public:
		Channel(std::string channelName, int fd);
		~Channel();
		bool			checkClientInChannel(int fd);
		int			getMyOperator();
		void			setMyOperator(int fd);
		std::string		getChannelName();
		std::vector<int>	getMyClientFdList();
		void			addMyClientList(int fd);
		void			removeClientList(int fd);
		std::vector<int>::iterator	findMyClientIt(int fd);
};

#endif
