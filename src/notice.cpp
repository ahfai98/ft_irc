#include "Server.hpp"

void Server::NOTICE(const std::string &cmd, int fd)
{
	Client *cli = getClientByFd(fd);
	if (!cli)
		return;
	std::vector<std::string> tokens = splitCommand(cmd);
	if (tokens.size() < 3)
		return;
	std::string targetStr = tokens[1];
	std::string message;
	if (!tokens[2].empty() && tokens[2][0] != ':')
		message = tokens[2];
	else
	{
		for (size_t i = 2; i < tokens.size(); ++i)
		{
			if (i > 2)
				message += " ";
			message += tokens[i];
		}
	}
	if (!message.empty() && message[0] == ':')
		message = message.substr(1);
	message = trim(message);
	if (message.empty())
		return;
	std::vector<std::string> targets = splitString(targetStr, ',');
	for (size_t i = 0; i < targets.size(); ++i)
	{
		std::string target = targets[i];
		if (target.empty())
			continue;
		if (target[0] == '#')
		{ // Channel
			std::string chName = target.substr(1);
			Channel *ch = getChannel(chName);
			if (!ch)
				continue;
			if (!ch->isInChannel(cli->getNickname()))
				continue;
			std::ostringstream oss;
			oss << ":" << cli->getPrefix() << " NOTICE " << target << " :" << message << "\r\n";
			ch->broadcastMessageExcept(*this, oss.str(), fd);
		}
		else
		{ // User
			Client *targetClient = getClientByNickname(target);
			if (!targetClient)
				continue;
			std::ostringstream oss;
			oss << ":" << cli->getPrefix() << " NOTICE " << target << " :" << message << "\r\n";
			sendResponse(targetClient->getSocketFd(), oss.str());
		}
	}
}
