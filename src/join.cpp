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
    if (!buff.empty())
        result.push_back(buff);
    return result;
}

// Helper: validate channel name
bool Server::isValidChannelName(const std::string &name)
{
    if (name.empty() || name[0] != '#')
        return false;
    // Only allow letters, digits, '-' and '_'
    for (size_t i = 1; i < name.size(); ++i)
    {
        char c = name[i];
        if (!(isalnum(c) || c == '-' || c == '_'))
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

    if (tokens.size() < 2)
    {
        sendResponse(fd, "461 " + nickname + " JOIN :Not enough parameters\r\n");
        return;
    }

    std::vector<std::string> channels_list = splitString(tokens[1], ',');
    std::vector<std::string> passwords;
    if (tokens.size() > 2)
        passwords = splitString(tokens[2], ',');

    // Align passwords vector
    while (passwords.size() < channels_list.size())
        passwords.push_back("");

    if (channels_list.size() > 10)
    {
        sendResponse(fd, "407 " + nickname + " JOIN :Too many channels\r\n");
        return;
    }

    for (size_t i = 0; i < channels_list.size(); ++i)
    {
        std::string chName = channels_list[i];
        std::string chPassword = passwords[i];

        if (!isValidChannelName(chName))
        {
            sendResponse(fd, "403 " + nickname + " " + chName + " :No such channel\r\n");
            continue;
        }

        if (!chPassword.empty() && !isValidKey(chPassword))
        {
            sendResponse(fd, "475 " + nickname + " " + chName + " :Invalid channel key\r\n");
            continue;
        }
        // Max channels per client
        if (cli->getJoinedChannels().size() >= 10)
        {
            sendResponse(fd, "405 " + nickname + " JOIN :You have joined too many channels\r\n");
            continue;
        }
        // Remove '#' for internal use
        if (chName[0] == '#')
            chName.erase(chName.begin());
        Channel *ch = getChannel(chName);
        if (ch) // Existing channel
        {
            if (ch->isInChannel(nickname))
                continue;
            if (ch->getInviteMode() && !ch->isInvited(nickname))
            {
                sendResponse(fd, "473 " + nickname + " " + chName + " :Cannot join channel (+i)\r\n");
                continue;
            }
            if (ch->getKeyMode() && ch->getChannelKey() != chPassword)
            {
                sendResponse(fd, "475 " + nickname + " " + chName + " :Cannot join channel (+k) - bad key\r\n");
                continue;
            }
            if (ch->getLimitMode() && ch->getChannelTotalClientCount() >= ch->getChannelLimit())
            {
                sendResponse(fd, "471 " + nickname + " " + chName + " :Cannot join channel (+l)\r\n");
                continue;
            }
            // Add client
            ch->addMember(cli);
            if (ch->isInvited(nickname))
                ch->removeInvited(nickname);
            // Send JOIN, NAMES, TOPIC messages
            std::string joinMsg = ":" + nickname + " JOIN #" + chName + "\r\n";
            ch->broadcastMessage(joinMsg); // broadcast JOIN to other members
            if (!ch->getTopicName().empty())
            {
                sendResponse(fd, ":ircserv 332 " + nickname + " #" + chName + " :" + ch->getTopicName() + "\r\n");
            }
            sendResponse(fd, ":ircserv 353 " + nickname + " = #" + chName + " :" + ch->getNamesList() + "\r\n");
            sendResponse(fd, ":ircserv 366 " + nickname + " #" + chName + " :End of NAMES list\r\n");
        }
        else // Create new channel
        {
            Channel *newCh = new Channel();
            newCh->setChannelName(chName);
            if (!chPassword.empty())
            {
                newCh->setChannelKey(chPassword);
                newCh->setKeyMode(true);
            }
            newCh->addOperator(cli);
            newCh->setTimeChannelCreated();
            addChannel(newCh);
            // Send messages
            std::string joinMsg = ":" + nickname + " JOIN #" + chName + "\r\n";
            newCh->broadcastMessage(joinMsg);
            sendResponse(fd, ":ircserv 353 " + nickname + " = #" + chName + " :" + newCh->getNamesList() + "\r\n");
            sendResponse(fd, ":ircserv 366 " + nickname + " #" + chName + " :End of NAMES list\r\n");
        }
    }
}
