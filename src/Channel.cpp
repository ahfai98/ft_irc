#include "Channel.hpp"

Channel::Channel()
:   inviteMode(false),
	topicMode(false),
	keyMode(false),
	limitMode(false),
	channelName(),
	channelLimit(-1),
	topicName(),
	topicSetter(),
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

void Channel::setTopicSetter(const std::string& setter){topicSetter = setter;}

void Channel::setTimeChannelCreated()
{
    time_t now = time(NULL);
    std::ostringstream oss;
    oss << now;
    timeChannelCreated = oss.str();
}

void Channel::setTimeTopicCreated()
{
	timeTopicCreated = time(NULL);
}

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

const time_t& Channel::getTimeTopicCreated() const{return timeTopicCreated;}

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

Client* Channel::getFirstMember()
{
	if (members.size() != 0)
		return members[0];
	return NULL;
}

Client* Channel::getOperatorByNickname(const std::string& nickname) { return getClientByNickname(operators, nickname); }

int 	Channel::getOperatorsCount()const {return operators.size();}
const std::string& Channel::getTopicSetter() const{ return topicSetter;}

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

//returns -1 if fail, otherwise returns how many bytes sent
void Channel::sendMessageToClients(Server& server, const std::vector<Client*>& clients, const std::string& msg, int excludeFd)
{
	for (size_t i = 0; i < clients.size(); ++i)
	{
		Client* cli = clients[i];
		if (!cli->getRegistered())
			continue;
		int fd = cli->getSocketFd();
		if (fd != excludeFd)
			server.sendResponse(fd, msg + "\r\n");
	}
}

void Channel::broadcastMessageToOperators(Server& server, const std::string& msg)
{
	sendMessageToClients(server, operators, msg);
}

void Channel::broadcastMessageToMembers(Server& server, const std::string& msg)
{
	sendMessageToClients(server, members, msg);
}

void Channel::broadcastMessage(Server& server, const std::string& msg)
{
	sendMessageToClients(server, operators, msg);
	sendMessageToClients(server, members, msg);
}

void Channel::broadcastMessageExcept(Server& server, const std::string& msg, int excludeFd)
{
	sendMessageToClients(server, operators, msg, excludeFd);
	sendMessageToClients(server, members, msg, excludeFd);
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

std::vector<Client*> Channel::getClientsList()
{
    std::vector<Client*> clientsList;
    clientsList.reserve(members.size() + operators.size());
    clientsList.insert(clientsList.end(), members.begin(), members.end());
    clientsList.insert(clientsList.end(), operators.begin(), operators.end());
    return clientsList;
}