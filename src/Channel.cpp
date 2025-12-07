#include "Channel.hpp"

Channel::Channel()
:   inviteMode(false),
	topicMode(false),
	keyMode(false),
	limitMode(false),
	channelName(),
	channelLimit(-1),
	topicName(),
	timeChannelCreated(),
	timeTopicCreated(),
	channelKey()
{
}

Channel::~Channel()
{
}

//Setters
void Channel::setInviteMode(bool value){inviteMode = value;}

void Channel::setTopicMode(bool value){topicMode = value;}

void Channel::setKeyMode(bool value){keyMode = value;}

void Channel::setLimitMode(bool value){limitMode = value;}

void Channel::setChannelName(const std::string& name){channelName = name;}

void Channel::setChannelLimit(int limit){channelLimit = limit;}

void Channel::setTopicName(const std::string& topic){topicName = topic;}

void Channel::setTimeChannelCreated(const std::string& time){timeChannelCreated = time;}

void Channel::setTimeTopicCreated(const std::string& time){timeTopicCreated =time;}

void Channel::setChannelKey(const std::string& key){channelKey =key ;}

//Getters
bool Channel::getInviteMode() const{return inviteMode;}

bool Channel::getTopicMode() const{return topicMode;}

bool Channel::getKeyMode() const{return keyMode;}

bool Channel::getLimitMode() const{return limitMode;}

const std::string& Channel::getChannelName() const{return channelName;}

int Channel::getChannelLimit() const{return channelLimit;}

int Channel::getChannelTotalClientCount() const{return members.size() + operators.size();}

const std::string& Channel::getTopicName() const{return topicName;}

const std::string& Channel::getTimeChannelCreated() const{return timeChannelCreated;}

const std::string& Channel::getTimeTopicCreated() const{return timeTopicCreated;}

const std::string& Channel::getChannelKey() const{return channelKey;}

bool Channel::isChannelOperator(const std::string& nickname) const
{
	std::vector<Client *>::const_iterator it;
	for (it = operators.begin(); it != operators.end(); ++it)
	{
		if ((*it)->getNickname() == nickname)
			return true;
	}
	return false;
}

bool Channel::isChannelMember(const std::string& nickname) const
{
	std::vector<Client *>::const_iterator it;
	for (it = members.begin(); it != members.end(); ++it)
	{
		if ((*it)->getNickname() == nickname)
			return true;
	}
	return false;
}

bool Channel::isInChannel(const std::string& nickname) const
{
	return (isChannelOperator(nickname) || isChannelMember(nickname));
}

//get operators and members name list for JOIN command
std::string Channel::getNamesList() const
{
	std::ostringstream oss;
	//Add operators
	for (size_t i = 0; i < operators.size(); ++i)
	{
		if (i > 0)
			oss << " ";
		oss << "@" << operators[i]->getNickname();
	}
	//Add space between operators and members
	if (!operators.empty() && !members.empty())
		oss << " ";
	//Add members
	for (size_t i = 0; i < members.size(); ++i)
	{
		if (i > 0)
			oss << " ";
		oss << members[i]->getNickname();
	}
	return oss.str();
}

Client* Channel::getClientByFd(std::vector<Client*>& vec, int fd)
{
	std::vector<Client*>::iterator it;
	for (it = vec.begin(); it != vec.end(); ++it)
	{
		if ((*it)->getSocketFd() == fd)
			return *it;
	}
	return NULL;
}

Client* Channel::getClientByNickname(std::vector<Client*>& vec, const std::string& nickname) 
{
	std::vector<Client*>::iterator it;
	for (it = vec.begin(); it != vec.end(); ++it)
	{
		if ((*it)->getNickname() == nickname)
			return *it;
	}
	return NULL;
}

Client* Channel::getMemberByFd(int fd) { return getClientByFd(members, fd); }
Client* Channel::getOperatorByFd(int fd) { return getClientByFd(operators, fd); }
Client* Channel::getMemberByNickname(const std::string& nickname) { return getClientByNickname(members, nickname); }
Client* Channel::getOperatorByNickname(const std::string& nickname) { return getClientByNickname(operators, nickname); }

//Utility Methods
void Channel::addMember(Client* c)
{
	if (isChannelMember(c->getNickname()))
		return;
	members.push_back(c);
}

void Channel::addOperator(Client* c)
{
	if (isChannelOperator(c->getNickname()))
		return;
	operators.push_back(c);
}

void Channel::removeMemberByFd(int fd)
{
	std::vector<Client *>::iterator it;
	for (it = members.begin(); it != members.end(); ++it)
	{
		if ((*it)->getSocketFd() == fd)
		{
			members.erase(it);
			return;
		}
	}
}

void Channel::removeOperatorByFd(int fd)
{
	std::vector<Client *>::iterator it;
	for (it = operators.begin(); it != operators.end(); ++it)
	{
		if ((*it)->getSocketFd() == fd)
		{
			operators.erase(it);
			return;
		}
	}
}

bool Channel::setAsOperator(const std::string& nickname)
{
	Client *c = getMemberByNickname(nickname);
	if (c != NULL)
	{
		removeMemberByFd(c->getSocketFd());
		addOperator(c);
		return true;
	}
	return false;
}

bool Channel::setAsMember(const std::string& nickname)
{
	Client *c = getOperatorByNickname(nickname);
	if (c != NULL)
	{
		removeOperatorByFd(c->getSocketFd());
		addMember(c);
		return true;
	}
	return false;
}

//ssize_t send(int sockfd, const void *buf, size_t len, int flags);
//returns -1 if fail, otherwise returns how many bytes sent
static void sendMessageToClients(const std::vector<Client*>& clients, const std::string& msg, int excludeFd = -1)
{
	for (size_t i = 0; i < clients.size(); ++i)
	{
		int fd = clients[i]->getSocketFd();
		if (fd != excludeFd)
		{
			if (send(fd, msg.c_str(), msg.size(), 0) == -1)
				std::cerr << "send() failed on fd " << fd << std::endl;
		}
	}
}

void Channel::broadcastMessageToOperators(const std::string& msg)
{
	sendMessageToClients(operators, msg);
}

void Channel::broadcastMessageToMembers(const std::string& msg)
{
	sendMessageToClients(members, msg);
}

void Channel::broadcastMessage(const std::string& msg)
{
	sendMessageToClients(operators, msg);
	sendMessageToClients(members, msg);
}

void Channel::broadcastMessageExcept(const std::string& msg, int excludeFd)
{
	sendMessageToClients(operators, msg, excludeFd);
	sendMessageToClients(members, msg, excludeFd);
}

void Channel::addInvited(const std::string& nickname)
{
	if(!isInvited(nickname))
		invitedNicknames.push_back(nickname);
}

void Channel::removeInvited(const std::string& nickname)
{
	std::vector<std::string>::iterator it;
	for (it = invitedNicknames.begin(); it != invitedNicknames.end(); ++it)
	{
		if (*it == nickname)
		{
			invitedNicknames.erase(it);
			return;
		}
	}
}

bool Channel::isInvited(const std::string& nickname) const
{
	std::vector<std::string>::const_iterator it;
	for (it = invitedNicknames.begin(); it != invitedNicknames.end(); ++it)
	{
		if (*it == nickname)
			return true;
	}
	return false;
}
