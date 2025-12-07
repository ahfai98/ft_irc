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
    Channel *channel = getChannel(channelName);
    std::string nickname = cli->getNickname();
    if (!channel)
    {
        sendResponse(fd, "403 " + channelName + " :No such channel\r\n");
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
        if(channel->getLimitMode())
        {
            modes += "l";
            params += " " + channel->getChannelLimit();
        }
        if(channel->isChannelOperator(nickname))
        {
            if(!channel->getKeyMode())
            {
                modes += "k";
                params += " " + channel->getChannelKey();
            }
        }
        sendResponse(fd, ":localhost 324 " + nickname + " " + channelName + " +" + modes + params);
        return;
    }

    std::string modeset = tokens[2];
    std::vector<std::string> params(tokens.begin() + 3, tokens.end());

    // Only operators can modify modes
    if (!channel->isChannelOperator(nickname))
    {
        sendResponse(fd, "482 " + nickname + " " + channelName + " :You're not channel operator\r\n");
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

            std::string msg = ":" + nickname + " MODE " + channelName + " " + current_op + "i\r\n";
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

            std::string msg = ":" + nickname + " MODE " + channelName + " " + current_op + "t\r\n";
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
                channel->setAsMember(targetNick);

            std::string msg = ":" + nickname + " MODE " + channelName + " " + current_op + "o " + targetNick + "\r\n";
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
            std::string msg = ":" + nickname + " MODE " + channelName + " " + current_op + "k";
            channel->broadcastMessageToMembers(msg);
            if (channel->getKeyMode())
                msg += " " + channel->getChannelKey();
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
                std::string limit = params[param_index++];
                if (!isValidLimit(limit))
                {
                    sendResponse(fd, "501 MODE :Invalid limit\r\n");
                    continue;
                }
                channel->setChannelLimit(atoi(limit.c_str()));
                channel->setLimitMode(true);
            }
            else if (current_op == '-')
            {
                channel->setChannelLimit(-1);
                channel->setLimitMode(false);
            }
            std::string msg = ":" + cli->getNickname() + " MODE " + channelName + " " + current_op + "l";
            if (channel->getLimitMode())
                msg += " " + channel->getChannelLimit();
            channel->broadcastMessage(msg);
            continue;
        }
        // Unknown mode
        sendResponse(fd, "472 " + std::string(1, mode) + " :Unknown mode\r\n");
    }
}
