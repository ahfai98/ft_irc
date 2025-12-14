#include "Server.hpp"

// Helper: split string by delimiter
std::vector<std::string> Server::splitString(const std::string &str, char delim)
{
    std::vector<std::string> result;
    std::string buff;
    for (size_t i = 0; i < str.size(); ++i)
    {
        if (str[i] == delim)
        {
            result.push_back(buff);
            buff.clear();
        }
        else
            buff += str[i];
    }
    result.push_back(buff);
    return result;
}

bool Server::isValidChannelName(const std::string &name)
{
    if (name.empty() || name[0] != '#')
        return false;
    if (name.size() > 50)
        return false;
    for (std::string::size_type i = 1; i < name.size(); ++i)
    {
        char c = name[i];
        //7 is Control + G
        if (c == ' ' || c == 7 || c == ',')
            return false;
    }
    return true;
}

void Server::JOIN(const std::string &cmd, int fd)
{
    Client *cli = getClientByFd(fd);
    if (!cli)
        return;
    std::string nickname = cli->getNickname();
    std::vector<std::string> tokens = splitString(cmd, ' ');
    if (tokens.size() == 2 && tokens[1] == "0")
    {
        std::vector<std::string> joinedChannels = cli->getJoinedChannels();
        if (joinedChannels.empty())
            return;
        for (size_t i = 0; i < joinedChannels.size(); ++i)
            PART("PART #" + joinedChannels[i], fd);
        return;
    }
    if (tokens.size() < 2)
    {
        sendResponse(fd, ":ircserv 461 " + nickname + " JOIN :Not enough parameters\r\n");
        return;
    }
    std::vector<std::string> channelsList = splitString(tokens[1], ',');
    std::vector<std::string> passwords;
    if (tokens.size() > 2)
        passwords = splitString(tokens[2], ',');
    // Align passwords vector
    while (passwords.size() < channelsList.size())
        passwords.push_back("");
    if (channelsList.size() > 10)
    {
        sendResponse(fd, ":ircserv 407 " + nickname + " JOIN :Too many channels\r\n");
        return;
    }
    for (size_t i = 0; i < channelsList.size(); ++i)
    {
        std::string chName = channelsList[i];
        std::string chPassword = passwords[i];
        if (!isValidChannelName(chName))
        {
            sendResponse(fd, ":ircserv 403 " + nickname + " " + chName + " :No such channel\r\n");
            continue;
        }
        if (!chPassword.empty() && !isValidChannelKey(chPassword))
        {
            sendResponse(fd, ":ircserv 475 " + nickname + " " + chName + " :Invalid channel key\r\n");
            continue;
        }
        if (cli->getJoinedChannels().size() >= 10)
        {
            sendResponse(fd, ":ircserv 405 " + nickname + " JOIN :You have joined too many channels\r\n");
            continue;
        }
        std::string internalChannelName = chName.substr(1);
        Channel *ch = getChannel(internalChannelName);
        if (ch) // Existing channel
        {
            if (ch->isInChannel(nickname))
                continue;
            if (ch->getInviteMode() && !ch->isInvited(nickname))
            {
                sendResponse(fd, ":ircserv 473 " + nickname + " " + chName + " :Cannot join channel (+i)\r\n");
                continue;
            }
            if (ch->getKeyMode() && ch->getChannelKey() != chPassword)
            {
                sendResponse(fd, ":ircserv 475 " + nickname + " " + chName + " :Cannot join channel (+k) - bad key\r\n");
                continue;
            }
            if (ch->getLimitMode() && ch->getChannelTotalClientCount() >= ch->getChannelLimit())
            {
                sendResponse(fd, ":ircserv 471 " + nickname + " " + chName + " :Cannot join channel (+l)\r\n");
                continue;
            }
            // Add client
            ch->addMember(cli);
            if (ch->isInvited(nickname))
                ch->removeInvited(nickname);
            // Send JOIN, NAMES, TOPIC messages
            std::string joinMsg = ":" + cli->getPrefix() + " JOIN " + chName + "\r\n";
            ch->broadcastMessage(joinMsg); // broadcast JOIN to other members
            if (!ch->getTopicName().empty())
            {
                sendResponse(fd, ":ircserv 332 " + nickname + " " + chName + " :" + ch->getTopicName() + "\r\n");
                std::ostringstream oss;
                oss << ":ircserv 333 " << nickname << " " << chName << " :" << ch->getTopicSetter() << " " << ch->getTimeTopicCreated() << "\r\n";
                sendResponse(fd, oss.str());
            }
            sendResponse(fd, ":ircserv 353 " + nickname + " = " + chName + " :" + ch->getNamesList() + "\r\n");
            sendResponse(fd, ":ircserv 366 " + nickname + " " + chName + " :End of NAMES list\r\n");
        }
        else // Create new channel
        {
            Channel *newCh = new Channel();
            newCh->setChannelName(internalChannelName);
            if (!chPassword.empty())
            {
                newCh->setChannelKey(chPassword);
                newCh->setKeyMode(true);
            }
            newCh->addOperator(cli);
            newCh->setTimeChannelCreated();
            addChannel(newCh);
            // Send messages
            std::string joinMsg = ":" + cli->getPrefix() + " JOIN " + chName + "\r\n";
            newCh->broadcastMessage(joinMsg);
            sendResponse(fd, ":ircserv 353 " + nickname + " = " + chName + " :" + newCh->getNamesList() + "\r\n");
            sendResponse(fd, ":ircserv 366 " + nickname + " " + chName + " :End of NAMES list\r\n");
        }
    }
}
