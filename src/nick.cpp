#include "Server.hpp"

bool Server::isValidNickname(const std::string &nickname)
{
	if (nickname.empty() || nickname.size() > 9)
		return false;
	const std::string forbidden = " ,*?!@";
	if (nickname.find_first_of(forbidden) != std::string::npos)
		return false;
	const std::string forbiddenFirst = "$:#&+";
	if (!nickname.empty() && forbiddenFirst.find(nickname[0]) != std::string::npos)
		return false;
	const std::string allowedStartSpecial = "[]\\`^{}";
	if (!std::isalpha(nickname[0]) && allowedStartSpecial.find(nickname[0]) == std::string::npos)
		return false;
	const std::string allowedMidSpecial = "_-[]\\`^{}";
	for (std::string::const_iterator it = nickname.begin() + 1; it != nickname.end(); ++it)
	{
		if (!std::isalnum(*it) && allowedMidSpecial.find(*it) == std::string::npos)
			return false;
	}
	return true;
}

// IRC case folding
static char ircFold(char c)
{
	switch (c)
	{
		case '{': return '[';
		case '}': return ']';
		case '|': return '\\';
		default: return std::tolower(c);
	}
}

static std::string ircLower(const std::string &nick)
{
	std::string out;
	out.reserve(nick.size());
	for (std::string::const_iterator it = nick.begin(); it != nick.end(); ++it)
		out.push_back(ircFold(*it));
	return out;
}

void Server::NICK(const std::string &cmd, int fd)
{
	Client *cli = getClientByFd(fd);
	if (!cli)
		return;
	std::vector<std::string> tokens = splitCommand(cmd);
	if (tokens.size() < 2)
	{
		sendResponse(fd, ":ircserv 431 * :No nickname given\r\n");
		return;
	}
	std::string newNickname = tokens[1];
	if (!newNickname.empty() && newNickname[0] == ':')
	{
		newNickname = newNickname.substr(1);
		newNickname = trim(newNickname);
	}
	if (!isValidNickname(newNickname))
	{
		sendResponse(fd, ":ircserv 432 * " + newNickname + " :Erroneous nickname\r\n");
		return;
	}
	std::string foldedNew = ircLower(newNickname);
	for (std::map<std::string, Client*>::iterator it = clientsByNickname.begin(); it != clientsByNickname.end(); ++it)
	{
		std::string existingFolded = ircLower(it->first);
		if (foldedNew == existingFolded && it->second != cli)
		{
			sendResponse(fd, ":ircserv 433 * " + newNickname + " :Nickname is already in use\r\n");
			return;
		}
	}
	//Remove old nickname from map
	std::string oldNickname = cli->getNickname();
	if (oldNickname != "*" && clientsByNickname[oldNickname] == cli)
		clientsByNickname.erase(oldNickname);
	//Add new nickname
	cli->setNickname(newNickname);
	clientsByNickname[newNickname] = cli;
	std::string oldMask = oldNickname == "*" ? "*" : oldNickname + "!" + cli->getUsername() + "@" + cli->getIpAddress();
	//Broadcast NICK change to channels
	if (cli->getRegistered())
	{
		sendResponse(fd, ":" + oldMask + " NICK :" + newNickname + "\r\n");
		for (std::map<std::string, Channel*>::iterator cit = channels.begin(); cit != channels.end(); ++cit)
		{
			Channel *ch = cit->second;
			//Update invite list
			if (ch->isInvited(oldNickname))
			{
				ch->removeInvited(oldNickname);
				ch->addInvited(newNickname);
			}
			// Broadcast message
			if (ch->isInChannel(newNickname))
				ch->broadcastMessageExcept(*this, ":" + oldMask + " NICK :" + newNickname + "\r\n", fd);
		}
	}
}
