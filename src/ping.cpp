#include "Server.hpp"

void Server::PING(const std::string &cmd, int fd)
{
	Client *cli = getClientByFd(fd);
	if (!cli)
		return;
	std::vector<std::string> tokens = splitCommand(cmd);
	if (tokens.size() < 2)
	{
		sendResponse(fd, ":ircserv 409 " + cli->getNickname() + " :No origin specified\r\n");
		return;
	}
	std::string token = tokens[1];
	std::string msg;
	if (!tokens[1].empty() && tokens[1][0] == ':')
	{
		for (size_t i = 1; i < tokens.size(); ++i)
		{
			if (i > 1)
				msg += " ";
			msg += tokens[i];
		}
		msg.erase(0, 1);
	}
	else
		msg = tokens[1];
	sendResponse(fd, ":ircserv PONG ircserv :" + msg + "\r\n");
}
