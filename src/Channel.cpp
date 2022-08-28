#include "../include/Channel.hpp"

Channel::Channel(std::string channelName, int fd) : _operator(fd), _channelName(channelName) {};

Channel::~Channel() {};

//매개변수 fd와 같은 fd를 갖는 클라이언트가 채널 내에 있는지 확인
bool				Channel::checkClientInChannel(int fd)
{
	std::vector<int>::iterator it = findMyClientIt(fd);
	if (it != _myClientFdList.end())
		return true;
	return false;
}

int					Channel::getMyOperator()
{
	return _operator;
}

void				Channel::setMyOperator(int fd)
{
	_operator = fd;
	return ;
}

std::string			getChannelName()
{
	return _channelName;
}

std::vector<int>	Channel::getMyClientFdList()
{
	return _myClientFdList;
}

void				Channel::addMyClientList(int fd)
{
	_myClientFdList.push_back(fd);
}

void				Channel::removeClientList(int fd)
{
	std::vector<int>::iterator it = findMyClientIt(fd);
	if (it != _myClientFdList.end())
		_myClientFdList.erase(it);
}

//매개변수 fd와 같은 fd를 갖는 클라이언트 객체를 반환
std::vector<int>::iterator Channel::findMyClientIt(int fd)
{
	std::vector<int>::iterator it = _myClientFdList.begin();
	while (it != _myClientFdList.end())
	{
		if ((*it) == fd)
			return (it);
		it++;
	}
	return (_myClientFdList.end());
}
