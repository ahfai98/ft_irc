#include "Server.hpp"

bool Server::isValidChannelKey(const std::string &key)
{
	if(key.empty())
		return false;
	for(size_t i = 0; i < key.size(); ++i)
	{
		if(key[i] < 33 || key[i] > 126)
			return false;
	}
	return true;
}

bool Server::isValidLimit(const std::string &limit)
{
	if (limit.empty())
		return false;
	int val = 0;
	const int MAX_LIMIT = 65535;
	for (size_t i = 0; i < limit.size(); ++i)
	{
		if (!isdigit(limit[i]))
			return false;
		int digit = limit[i] - '0';
		// Check overflow before multiplying by 10
		if (val > (MAX_LIMIT - digit) / 10)
			return false;
		val = val * 10 + digit;
	}
	return val > 0;
}

void Server::MODE(const std::string &cmd, int fd)
{
	Client *cli = getClientByFd(fd);
	if (!cli)
		return;
	std::string nickname = cli->getNickname();
	std::vector<std::string> tokens = splitCommand(cmd);
	if (tokens.size() < 2)
	{
		sendResponse(fd, ":ircserv 461 " + nickname + " MODE :Not enough parameters\r\n");
		return;
	}

	// IF TARGET IS A NICKNAME (User Mode)
	if (!tokens[1].empty() && tokens[1][0] != '#')
	{
		Client *targetCli = getClientByNickname(tokens[1]);
		if (!targetCli)
		{
			sendResponse(fd, ":ircserv 401 " + tokens[1] + " :No such nick/channel\r\n");
			return;
		}
		// If they are asking for their own mode or setting +i
		// Just echo back a success to keep the client happy
		if (tokens.size() == 2)
			sendResponse(fd, ":ircserv 221 " + tokens[1] + " +i\r\n");
		else
			sendResponse(fd, ":" + tokens[1] + " MODE " + tokens[1] + " " + tokens[2] + "\r\n");
		return;
	}

	std::string chName = tokens[1];
	if (!isValidChannelName(chName))
	{
		sendResponse(fd, ":ircserv 403 " + nickname + " " + chName + " :No such channel\r\n");
		return;
	}
	std::string internalChannelName = chName.substr(1);
	Channel *ch = getChannel(internalChannelName);
	if (!ch)
	{
		sendResponse(fd, ":ircserv 403 " + nickname + " " + chName + " :No such channel\r\n");
		return;
	}
	//Query
	if (tokens.size() == 2)
	{
		std::string modeChars = "";
		std::vector<std::string> qParams;

		// Check each mode. Note: 'n' and 't' are often default on many servers.
		if (ch->getInviteMode())
			modeChars += "i";
		if (ch->getTopicMode())
			modeChars += "t";
		if (ch->getKeyMode())
		{
			modeChars += "k";
			qParams.push_back(ch->getChannelKey());
		}
		if (ch->getLimitMode())
		{
			modeChars += "l";
			std::ostringstream oss;
			oss << ch->getChannelLimit();
			qParams.push_back(oss.str());
		}

		// If no modes are set, IRC standard usually expects at least a '+' or '+n'
		// Irssi specifically dislikes a '+' with nothing following it.
		std::string modeStr = modeChars.empty() ? "+" : "+" + modeChars;

		std::string fullResponse = ":ircserv 324 " + nickname + " " + chName + " " + modeStr;
		
		// Append parameters (key/limit) if they exist
		for (size_t i = 0; i < qParams.size(); ++i)
		{
			fullResponse += " " + qParams[i];
		}
		fullResponse += "\r\n";

		// Send RPL_CHANNELMODEIS (324)
		sendResponse(fd, fullResponse);

		// Send RPL_CREATIONTIME (329) 
		// Ensure getTimeChannelCreated() returns a string of digits (Unix Timestamp)
		std::string creationTime = ch->getTimeChannelCreated();
		sendResponse(fd, ":ircserv 329 " + nickname + " " + chName + " " + creationTime + "\r\n");
		
		return;
	}

	// 1. Check if the client is asking for the Ban List
	// This catches 'b', '+b', and 'b '
	if (tokens.size() >= 3 && tokens[2].find('b') != std::string::npos)
	{
		sendResponse(fd, ":ircserv 368 " + nickname + " " + chName + " :End of channel ban list\r\n");
		return;
	}

	// 2. Check if the client is asking for the Exception List
	if (tokens.size() == 3 && tokens[2] == "e")
	{
		sendResponse(fd, ":ircserv 349 " + nickname + " " + chName + " :End of channel exception list\r\n");
		return;
	}

	// 3. Check if the client is asking for the Invite-Exception List
	if (tokens.size() == 3 && tokens[2] == "I")
	{
		sendResponse(fd, ":ircserv 347 " + nickname + " " + chName + " :End of channel invite exception list\r\n");
		return;
	}

	std::string modeset = tokens[2];
	std::vector<std::string> params(tokens.begin() + 3, tokens.end());
	// Check is operator
	if (!ch->isChannelOperator(nickname))
	{
		sendResponse(fd, ":ircserv 482 " + nickname + " " + chName + " :You're not channel operator\r\n");
		return;
	}
	char current_op = '\0';
	char last_op = '\0';
	size_t param_index = 0;
	std::string outModes;
	std::vector<std::string> outParams;
	size_t paramModeCount = 0;
	const size_t MAX_PARAM_MODES = 3;
	for (size_t i = 0; i < modeset.size(); ++i)
	{
		char mode = modeset[i];
		if (mode == '+' || mode == '-')
		{
			current_op = mode;
			continue;
		}
		if (current_op == '\0')
			continue;
		//INVITE (+i / -i) 
		if (mode == 'i')
		{
			if (current_op != last_op)
			{
				outModes += current_op;
				last_op = current_op;
			}
			outModes += 'i';
			ch->setInviteMode(current_op == '+');
			continue;
		}
		//TOPIC (+t / -t) 
		if (mode == 't')
		{
			if (current_op != last_op)
			{
				outModes += current_op;
				last_op = current_op;
			}
			outModes += 't';
			ch->setTopicMode(current_op == '+');
			continue;
		}
		//OPERATOR (+o / -o) 
		if (mode == 'o')
		{
			if (param_index >= params.size())
			{
				sendResponse(fd, ":ircserv 461 " + nickname + " MODE :Not enough parameters\r\n");
				continue;
			}
			//Discard parameter if already more than max 3
			if (paramModeCount >= MAX_PARAM_MODES)
			{
				param_index++;
				continue;
			}
			std::string targetNick = params[param_index++];
			Client *target = getClientByNickname(targetNick);
			if (!target || !ch->isInChannel(targetNick))
			{
				sendResponse(fd, ":ircserv 441 " + nickname + " " + targetNick + " " + chName + " :They aren't on that channel\r\n");
				continue;
			}
			if (current_op == '+')
				ch->setAsOperator(targetNick);
			else
				ch->setAsMember(targetNick);
			paramModeCount++;
			if (current_op != last_op)
			{
				outModes += current_op;
				last_op = current_op;
			}
			outModes += 'o';
			outParams.push_back(targetNick);
			continue;
		}
		//CHANNEL KEY (+k / -k) 
		if (mode == 'k')
		{
			if (current_op == '+')
			{
				if (param_index >= params.size())
				{
					sendResponse(fd, ":ircserv 461 " + nickname + " MODE :Not enough parameters\r\n");
					continue;
				}
				//Discard parameter if more than max 3
				if (paramModeCount >= MAX_PARAM_MODES)
				{
					param_index++;
					continue;
				}
				std::string key = params[param_index++];
				if (!isValidChannelKey(key))
				{
					sendResponse(fd, ":ircserv 472 " + nickname + " k :Invalid channel key\r\n");
					continue;
				}
				ch->setChannelKey(key);
				ch->setKeyMode(true);
				paramModeCount++;
				if (current_op != last_op)
				{
					outModes += current_op;
					last_op = current_op;
				}
				outModes += 'k';
				outParams.push_back(key);
			}
			else
			{
				ch->setChannelKey("");
				ch->setKeyMode(false);
				if (current_op != last_op)
				{
					outModes += current_op;
					last_op = current_op;
				}
				outModes += 'k';
			}
			continue;
		}
		//USER LIMIT (+l / -l) 
		if (mode == 'l')
		{
			if (current_op == '+')
			{
				if (param_index >= params.size())
				{
					sendResponse(fd, ":ircserv 461 " + nickname + " MODE :Not enough parameters\r\n");
					continue;
				}
				//Discard parameter if more than max 3
				if (paramModeCount >= MAX_PARAM_MODES)
				{
					param_index++;
					continue;
				}
				std::string limitStr = params[param_index++];
				if (!isValidLimit(limitStr))
				{
					sendResponse(fd, ":ircserv 501 " + nickname + " MODE :Invalid limit\r\n");
					continue;
				}
				int limit = atoi(limitStr.c_str());
				ch->setChannelLimit(limit);
				ch->setLimitMode(true);
				paramModeCount++;
				if (current_op != last_op)
				{
					outModes += current_op;
					last_op = current_op;
				}
				outModes += 'l';
				outParams.push_back(limitStr);
			}
			else
			{
				ch->setChannelLimit(-1);
				ch->setLimitMode(false);
				if (current_op != last_op)
				{
					outModes += current_op;
					last_op = current_op;
				}
				outModes += 'l';
			}
			continue;
		}
		//UNKNOWN MODE 
		sendResponse(fd, ":ircserv 472 " + nickname + " " + std::string(1, mode) + " :is unknown mode char to me\r\n");
	}
	//FINAL BROADCAST (ONE MESSAGE) 
	if (!outModes.empty())
	{
		std::string msg = ":" + cli->getPrefix() + " MODE " + chName + " " + outModes;
		for (size_t i = 0; i < outParams.size(); ++i)
			msg += " " + outParams[i];
		msg += "\r\n";
		ch->broadcastMessage(*this, msg);
	}
}