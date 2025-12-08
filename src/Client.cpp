#include "Client.hpp"

Client::Client(int fd)
:   socketFd(fd),
	isPasswordAuthenticated(false),
	isRegistered(false),
	username(""),
	nickname("*"),
	receiveBuffer(""),
	ipAddress("")
{
}

Client::~Client()
{
}

int Client::getSocketFd() const {return socketFd;}

bool Client::getPasswordAuthenticated() const {return isPasswordAuthenticated;}

bool Client::getRegistered() const {return isRegistered;}

std::string Client::getUsername() const {return username;}

std::string Client::getNickname() const{return nickname;}

std::string& Client::getBuffer() {return receiveBuffer;}

const std::vector<std::string>& Client::getJoinedChannels() const {return joinedChannels;}

void Client::setSocketFd(int fd) {socketFd = fd;}

void Client::setPasswordAuthenticated(bool value){isPasswordAuthenticated = value;}

void Client::setRegistered(bool value){isRegistered = value;}

void Client::setUsername(const std::string& username){this->username = username;}

void Client::setNickname(const std::string& nickname){this->nickname = nickname;}

void Client::setBuffer(const std::string& buf){receiveBuffer = buf;}

void Client::setIpAddress(const std::string& ipAddress){this->ipAddress = ipAddress;}

void Client::clearBuffer(){receiveBuffer.clear();}

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

std::string Client::getPrefix() const
{
    return getNickname() + "!" + getUsername() + "@" + "localhost";
}