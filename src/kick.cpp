#include "Server.hpp"

void Server::KICK(const std::string &cmd, int fd)
{
	Client *cli = getClientByFd(fd);
	if (!cli)
		return;
	std::string nickname = cli->getNickname();
	std::vector<std::string> tokens = splitCommand(cmd);
	if (tokens.size() < 3)
	{
		sendResponse(fd, ":ircserv 461 " + nickname + " KICK :Not enough parameters\r\n");
		return;
	}
	std::string channelsList = tokens[1];
	std::string usersList = tokens[2];
	std::string reason = "";
	if (tokens.size() > 3)
	{
		if (!tokens[3].empty() && tokens[3][0] == ':')
		{
			for (size_t i = 3; i < tokens.size(); ++i)
			{
				if (i > 3)
					reason += " ";
				reason += tokens[i];
			}
			if (!reason.empty())
			{
				reason = reason.substr(1);
				reason = trim(reason);
			}
		}
		else if (!tokens[3].empty())
			reason = trim(tokens[3]);
	}
	std::vector<std::string> channels = splitString(channelsList, ',');
	std::vector<std::string> users = splitString(usersList, ',');

	//Check if theres only 1 user for channel(s), or multiple user for multiple channels
	if (!(users.size() == 1 || users.size() == channels.size()))
	{
		sendResponse(fd, ":ircserv 461 " + nickname + " KICK :Not enough parameters\r\n");
		return;
	}
	for (size_t i = 0; i < channels.size(); ++i)
	{
		std::string chName = channels[i];
		if (!isValidChannelName(chName))
		{
			sendResponse(fd, ":ircserv 403 " + nickname + " " + chName + " :No such channel\r\n");
			continue;
		}
		std::string internalChannelName = chName.substr(1);
		Channel *ch = getChannel(internalChannelName);
		if (!ch)
		{
			sendResponse(fd, ":ircserv 403 " + nickname + " " + chName + " :No such channel\r\n");
			continue;
		}
		if (!ch->isInChannel(nickname))
		{
			sendResponse(fd, ":ircserv 442 " + nickname + " " + chName + " :You're not on that channel\r\n");
			continue;
		}

		if (!ch->isChannelOperator(nickname))
		{
			sendResponse(fd, ":ircserv 482 " + nickname + " " + chName + " :You're not channel operator\r\n");
			continue;
		}
		std::string targetNick;
		if(users.size() == 1)
			targetNick = users[0];
		else
			targetNick = users[i];
		if (!ch->isInChannel(targetNick))
		{
			sendResponse(fd, ":ircserv 441 " + nickname + " " + targetNick + " " + chName + " :They aren't on that channel\r\n");
			continue;
		}
		Client *target = getClientByNickname(targetNick);
		if (!target)
		{
			sendResponse(fd, ":ircserv 401 " + nickname + " " + targetNick + " :No such nick\r\n");
			continue;
		}
		if (targetNick == nickname)
		{
			sendResponse(fd, ":ircserv 482 " + nickname + " " + chName + " :You cannot kick yourself\r\n");
			continue;
		}
		//Build KICK message
		if (reason.empty())
			reason = "Kicked by " + nickname;
		std::ostringstream oss;
		oss << ":" << cli->getPrefix() << " KICK " << chName << " " << targetNick << " :" << reason << "\r\n";
		ch->broadcastMessage(*this, oss.str());
		target->removeJoinedChannels(internalChannelName);
		//Remove target from channel
		if (ch->isChannelMember(targetNick))
			ch->removeMemberByFd(target->getSocketFd());
		else
			ch->removeOperatorByFd(target->getSocketFd());
	}
}
