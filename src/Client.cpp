#include "../include/Client.hpp"
#include <iostream>


Client::Client(int clientFd) : _clientFd(clientFd), _regist(0)
{
}

Client::~Client()
{
}

std::string Client::getMsgBuffer()


