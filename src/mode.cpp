#include "Server.hpp"

bool Server::isValidKey(const std::string &key)
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
    if(limit.empty())
        return false;
    for(size_t i = 0; i < limit.size(); ++i)
        if(!isdigit(limit[i]))
            return false;
    int val = atoi(limit.c_str());
    return val > 0;
}

void Server::MODE(const std::string &cmd, int fd)
{
    Client *cli = getClientByFd(fd);
    if (!cli)
        return;
    std::vector<std::string> tokens = splitCommand(cmd);
    if (tokens.size() < 2)
    {
        sendResponse(fd, "461 MODE :Not enough parameters\r\n");
        return;
    }

    std::string channelName = tokens[1];

    // Validate channel name starts with '#'
    if (!isValidChannelName(channelName))
    {
        sendResponse(fd, "403 " + channelName + " :No such channel\r\n");
        return;
    }

    // Strip '#' for internal use
    std::string internalChannelName = channelName.substr(1);


    Channel *channel = getChannel(internalChannelName);
    std::string nickname = cli->getNickname();
    if (!channel)
    {
        sendResponse(fd, "403 " + nickname + " #" + internalChannelName + " :No such channel\r\n");
        return;
    }
    // Query current channel modes if only 2 tokens (MODE #channel)
    if (tokens.size() == 2)
    {
        std::string modes = "";
        std::string params = "";

        if(channel->getInviteMode())
            modes += "i";
        if(channel->getTopicMode())
            modes += "t";
        if(channel->getKeyMode() && channel->isChannelOperator(nickname))
        {
            modes += "k";
            params += " " + channel->getChannelKey();
        }
        if(channel->getLimitMode())
        {
            modes += "l";
            int limit = channel->getChannelLimit();
            std::ostringstream oss;
            oss << limit;
            std::string limit_str = oss.str();
            params += " " + limit_str;
        }
        sendResponse(fd, ":localhost 324 " + nickname + " #" + internalChannelName + " " + modes + params + "\r\n");
        return;
    }

    std::string modeset = tokens[2];
    std::vector<std::string> params(tokens.begin() + 3, tokens.end());

    // Only operators can modify modes
    if (!channel->isChannelOperator(nickname))
    {
        sendResponse(fd, "482 " + nickname + " #" + internalChannelName + " :You're not channel operator\r\n");
        return;
    }

    char current_op = '\0';
    size_t param_index = 0;
    for (size_t i = 0; i < modeset.size(); ++i)
    {
        char mode = modeset[i];
        if (mode == '+' || mode == '-')
        {
            current_op = mode;
            continue;
        }

        // Invite-only mode (+i/-i)
        if (mode == 'i')
        {
            if (current_op == '+')
                channel->setInviteMode(true);
            else if (current_op == '-')
                channel->setInviteMode(false);

            std::string msg = ":" + nickname + " MODE #" + internalChannelName + " " + current_op + "i\r\n";
            channel->broadcastMessage(msg);
            continue;
        }
        // Topic protection (+t/-t)
        if (mode == 't')
        {
            if (current_op == '+')
                channel->setTopicMode(true);
            else if (current_op == '-')
                channel->setTopicMode(false);

            std::string msg = ":" + nickname + " MODE #" + internalChannelName + " " + current_op + "t\r\n";
            channel->broadcastMessage(msg);
            continue;
        }
        // Operator (+o/-o)
        if (mode == 'o')
        {
            if (param_index >= params.size())
            {
                sendResponse(fd, "461 MODE :Not enough parameters for +o/-o\r\n");
                continue;
            }

            std::string targetNick = params[param_index++];
            Client *target = getClientByNickname(targetNick);

            if (!target || !channel->isInChannel(targetNick))
            {
                sendResponse(fd, "441 " + nickname + " " + targetNick + " :They aren't on that channel\r\n");
                continue;
            }
            if (current_op == '+')
                channel->setAsOperator(targetNick);
            else
            {
                if (targetNick == nickname)
                {
                    if (channel->getOperatorsCount() == 1 && channel->getChannelTotalClientCount() != 1)
                    {
                        Client* promote = channel->getFirstMember();
                        channel->setAsOperator(promote->getNickname());
                        channel->setAsMember(targetNick);
                        // send MODE broadcast
                        std::string msg = ":localhost " + nickname + " MODE #" + internalChannelName + " +o " + promote->getNickname() + "\r\n";
                        channel->broadcastMessage(msg);
                    }
                    else if(channel->getOperatorsCount() == 1 && channel->getChannelTotalClientCount() == 1)
                    {
                        sendResponse(fd, "441 " + nickname + " " + targetNick + " :Channel must have one operator. Cannot demote.\r\n");
                        continue;
                    }
                }
                else
                    channel->setAsMember(targetNick);
            }
            std::string msg = ":" + nickname + " MODE #" + internalChannelName + " " + current_op + "o " + targetNick + "\r\n";
            channel->broadcastMessage(msg);
            continue;
        }

        // Channel key/password (+k/-k)
        if (mode == 'k')
        {
            if (current_op == '+')
            {
                if (param_index >= params.size())
                {
                    sendResponse(fd, "461 MODE :Not enough parameters for +k\r\n");
                    continue;
                }
                std::string key = params[param_index++];
                if(!isValidKey(key))
                {
                    sendResponse(fd, "472 " + key + " :Invalid channel key\r\n");
                    continue;
                }
                channel->setChannelKey(key);
                channel->setKeyMode(true);
            }
            else if (current_op == '-')
            {
                channel->setChannelKey("");
                channel->setKeyMode(false);
            }
            std::string msg = ":" + nickname + " MODE #" + internalChannelName + " " + current_op + "k\r\n";
            channel->broadcastMessageToMembers(msg);
            if (channel->getKeyMode())
                msg += " " + channel->getChannelKey() + "\r\n";
            channel->broadcastMessageToOperators(msg);
            continue;
        }
        // User limit (+l/-l)
        if (mode == 'l')
        {
            if (current_op == '+')
            {
                if (param_index >= params.size())
                {
                    sendResponse(fd, "461 MODE :Not enough parameters for +l\r\n");
                    continue;
                }
                std::string limitStr = params[param_index++];
                if (!isValidLimit(limitStr))
                {
                    sendResponse(fd, "501 MODE :Invalid limit\r\n");
                    continue;
                }
                int limit = atoi(limitStr.c_str());
                channel->setChannelLimit(limit);
                channel->setLimitMode(true);
                std::ostringstream oss;
                oss << limit;
                std::string msg = ":" + nickname + " MODE #" + internalChannelName + " " + current_op + "l " + oss.str() + "\r\n";
                channel->broadcastMessage(msg);
            }
            else if (current_op == '-')
            {
                channel->setChannelLimit(-1);
                channel->setLimitMode(false);
                std::string msg = ":" + nickname + " MODE #" + internalChannelName + " -l\r\n";
                channel->broadcastMessage(msg);
            }
            continue;
        }
        // Unknown mode
        sendResponse(fd, "472 " + std::string(1, mode) + " :Unknown mode\r\n");
    }
}
