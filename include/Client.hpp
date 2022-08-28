#ifndef CLIENT_HPP
#define CLIENT_HPP

#include <vector>
//#include "Channel.hpp"

#define PASS 1
#define NICK 2
#define USER 4
#define REGI 8

class Channel;

class Client
{
	private :
		std::string _nickName;
		std::string _userName;
		std::string _hostName;
		std::stirng _serverName;
		std::string _realName;
		int _clientFd;
		unsigned char _regist;
		std::vector<std::string> _myChannelList;
		std::string _msgBuffer;
		std::string _recvBuffer;
		
	public :
		Client(int clientFd);
		~Client();
		bool isRegist();
		void appendMsgBuffer(std::string msgBuffer);
		void appendRecvBuffer(std::string recvBuffer);
		void clearMsgBuffer();
		void clearRecvBuffer();
		void setUser(std::string userName, std::string hostName, std::string serverName);
		void setNickName(std::string nickName);
		void setRegist(int bit);
		int get ClientFd();
		unsigned char getRegist();
		std::string getMsgBuffer();
		std::string getNickName();
		std::string getUserName();
		std::string getHostName();
		std::string getServerName();
		std::string getRealName();
		std::string &getRecvBuffer();
		std::vecotr<std::string> &getMyChannelList();
		void addChannelList(std::string channelName);
		void removeChannel(std::string serverName);
		std::vector<std::string>::iterator findMyChannelIt(std::string item);
};

#endif


}
