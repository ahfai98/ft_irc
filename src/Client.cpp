#include "Client.hpp"

Client::Client(int fd)
:   socketFd(fd),
	isPasswordAuthenticated(false),
	isRegistered(false),
	isQuitting(false),
	username(""),
	realname(""),
	nickname("*"),
	receiveBuffer(""),
	sendBuffer(""),
	ipAddress("")
{
}

Client::~Client()
{
}

int Client::getSocketFd() const {return socketFd;}

bool Client::getPasswordAuthenticated() const {return isPasswordAuthenticated;}

bool Client::getRegistered() const {return isRegistered;}

bool Client::getQuitting() const {return isQuitting;}

std::string Client::getUsername() const {return username;}

std::string Client::getRealname() const {return realname;}

std::string Client::getNickname() const{return nickname;}

std::string& Client::getReceiveBuffer() {return receiveBuffer;}

std::string& Client::getSendBuffer() {return sendBuffer;}

const std::vector<std::string>& Client::getJoinedChannels() const {return joinedChannels;}

void Client::setSocketFd(int fd) {socketFd = fd;}

void Client::setPasswordAuthenticated(bool value){isPasswordAuthenticated = value;}

void Client::setRegistered(bool value){isRegistered = value;}

void Client::setQuitting(bool value){isQuitting = value;}

void Client::setUsername(const std::string& username){this->username = username;}

void Client::setRealname(const std::string& realname){this->realname = realname;}

void Client::setNickname(const std::string& nickname){this->nickname = nickname;}

void Client::setReceiveBuffer(const std::string& buf){receiveBuffer = buf;}

void Client::setSendBuffer(const std::string& buf){sendBuffer = buf;}

void Client::setIpAddress(const std::string& ipAddress){this->ipAddress = ipAddress;}

void Client::clearReceiveBuffer(){receiveBuffer.clear();}

void Client::clearSendBuffer(){sendBuffer.clear();}

bool Client::isInChannel(const std::string& name) const
{
	std::vector<std::string>::const_iterator it;
	for (it = joinedChannels.begin(); it != joinedChannels.end(); ++it)
	{
		if (*it == name)
			return true;
	}
	return false;
}

void Client::addJoinedChannels(const std::string& name)
{
	for (int i = joinedChannels.size() - 1; i >= 0; --i)
    {
        if (joinedChannels[i] == name)
			return;
    }
	joinedChannels.push_back(name);
}

void Client::removeJoinedChannels(const std::string& name)
{
    for (int i = joinedChannels.size() - 1; i >= 0; --i)
    {
        if (joinedChannels[i] == name)
		{
            joinedChannels.erase(joinedChannels.begin() + i);
			break;
		}
    }
}

std::string Client::getPrefix() const
{
    return getNickname() + "!" + getUsername() + "@" + "localhost";
}

std::string Client::getIpAddress() const{return ipAddress;}