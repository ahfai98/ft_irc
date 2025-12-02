#include "Client.hpp"

Client::Client(int fd)
:   socketFd(fd),
	isPasswordAuthenticated(false),
	isFullyAuthenticated(false),
	username(""),
	nickname(""),
	receiveBuffer(""),
	ipAddress("")
{
}

Client::~Client()
{
}

int Client::getSocketFd() const
{
	return socketFd;
}

bool Client::getPasswordAuthenticated() const
{
	return isPasswordAuthenticated;
}

bool Client::getFullyAuthenticated() const
{
	return isFullyAuthenticated;
}

bool Client::isInvitedToChannel(const std::string &name) const
{
	std::vector<std::string>::const_iterator it;
	for (it = invitedChannels.begin(); it != invitedChannels.end(); ++it)
	{
		if (*it == name)
			return true;
	}
	return false;
}


std::string Client::getUsername() const
{
	return username;
}

std::string Client::getNickname() const
{
	return nickname;
}
std::string Client::getBuffer() const
{
	return receiveBuffer;
}

const std::vector<std::string>& Client::getInvitedChannels() const
{
	return invitedChannels;
}

const std::vector<std::string>& Client::getJoinedChannels() const
{
	return joinedChannels;
}

void Client::setSocketFd(int fd)
{
	socketFd = fd;
}

void Client::setPasswordAuthenticated(bool value)
{
	isPasswordAuthenticated = value;
}

void Client::setFullyAuthenticated(bool value)
{
	isFullyAuthenticated = value;
}

void Client::setUsername(const std::string& username)
{
	this->username = username;
}

void Client::setNickname(const std::string& nickname)
{
	this->nickname = nickname;
}

void Client::setBuffer(const std::string& buf)
{
	receiveBuffer = buf;
}

void Client::setIpAddress(const std::string& ipAddress)
{
	this->ipAddress = ipAddress;
}

void Client::clearBuffer()
{
	receiveBuffer.clear();
}

void Client::addChannelInvites(const std::string& name)
{
	invitedChannels.push_back(name);
}

void Client::removeChannelInvites(const std::string& name)
{
	std::vector<std::string>::iterator it;
	for (it = invitedChannels.begin(); it != invitedChannels.end(); ++it)
	{
		if (*it == name)
		{
			invitedChannels.erase(it);
			return;
		}
	}
}

void Client::addJoinedChannels(const std::string& name)
{
	joinedChannels.push_back(name);
}

void Client::removeJoinedChannels(const std::string& name)
{
	std::vector<std::string>::iterator it;
	for (it = joinedChannels.begin(); it != joinedChannels.end(); ++it)
	{
		if (*it == name)
		{
			joinedChannels.erase(it);
			return;
		}
	}
}

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
