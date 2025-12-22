#include "Server.hpp"

void Server::INVITE(const std::string &cmd, int fd)
{
	Client *inviter = getClientByFd(fd);
	if (!inviter)
		return;
	std::string inviterNick = inviter->getNickname();
	std::vector<std::string> tokens = splitCommand(cmd);
	if (tokens.size() < 3)
	{
		sendResponse(fd, ":ircserv 461 " + inviterNick + " INVITE :Not enough parameters\r\n");
		return;
	}
	std::string targetNick = tokens[1];
	std::string chName = tokens[2];
	Client *target = getClientByNickname(targetNick);
	if (!target)
	{
		sendResponse(fd, ":ircserv 401 " + inviterNick + " " + targetNick + " :No such nick/channel\r\n");
		return;
	}
	if (!isValidChannelName(chName))
	{
		sendResponse(fd, ":ircserv 403 " + inviterNick + " " + chName + " :No such channel\r\n");
		return;
	}
	std::string internalChannelName = chName.substr(1);
	Channel *ch = getChannel(internalChannelName);
	if (!ch)
	{
		sendResponse(fd, ":ircserv 403 " + inviterNick + " " + chName + " :No such channel\r\n");
		return;
	}
	if (!ch->isInChannel(inviterNick))
	{
		sendResponse(fd, ":ircserv 442 " + inviterNick + " " + chName + " :You're not on that channel\r\n");
		return;
	}
	if (ch->getInviteMode() && !ch->isChannelOperator(inviterNick))
	{
		sendResponse(fd, ":ircserv 482 " + inviterNick + " " + chName + " :You're not channel operator\r\n");
		return;
	}
	if (ch->isInChannel(targetNick))
	{
		sendResponse(fd, ":ircserv 443 " + inviterNick + " " + targetNick + " " + chName + " :is already on channel\r\n");
		return;
	}
	ch->addInvited(targetNick);
	//Notify inviter
	sendResponse(fd, ":ircserv 341 " + inviterNick + " " + targetNick + " " + chName + "\r\n");
	//Notify target
	std::string msg = ":" + inviter->getPrefix() + " INVITE " + targetNick + " " + chName + "\r\n";
	sendResponse(target->getSocketFd(), msg);
}
