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
        std::string modes = "";
        std::string params = "";

        if(ch->getInviteMode())
            modes += "i";
        if(ch->getTopicMode())
            modes += "t";
        if(ch->getKeyMode())
            modes += "k";
        if(ch->getLimitMode())
            modes += "l";
        if(ch->getKeyMode() && ch->isChannelOperator(nickname))
            params += ch->getChannelKey();
        if(ch->getLimitMode())
        {
            int limit = ch->getChannelLimit();
            std::ostringstream oss;
            oss << limit;
            std::string limit_str = oss.str();
            params += " " + limit_str;
        }
        sendResponse(fd, ":ircserv 324 " + nickname + " " + chName + " +" + modes + params + "\r\n");
        sendResponse(fd, ":ircserv 329 " + nickname + " " + chName + " timestamp channel created\r\n");
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
