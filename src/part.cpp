#include "Server.hpp"

void Server::PART(const std::string &cmd, int fd)
{
    Client *cli = getClientByFd(fd);
    if (!cli)
        return;
    std::vector<std::string> tokens = splitCommand(cmd);
    std::string nickname = cli->getNickname();
    if (tokens.size() < 2)
    {
        sendResponse(fd, ":ircserv 461 " + nickname + " PART :Not enough parameters\r\n");
        return;
    }
    std::vector<std::string> channels = splitString(tokens[1], ',');
    std::string partMessage = nickname + " has left the channel";
    if (tokens.size() > 2)
    {
        if (tokens[2][0] != ':')
            partMessage = tokens[2];
        else
        {
            for (size_t i = 2; i < tokens.size(); ++i)
            {
                if (i > 2)
                    partMessage += " ";
                partMessage += tokens[i];
            }
        }
        if (!partMessage.empty() && partMessage[0] == ':')
            partMessage = partMessage.substr(1);
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
        std::ostringstream oss;
        oss << ":" << cli->getPrefix() << " PART " << chName << " :" << partMessage << "\r\n";
        cli->removeJoinedChannels(internalChannelName);
        ch->broadcastMessage(*this, oss.str());
        ch->removeOperatorByFd(fd);
        ch->removeMemberByFd(fd);
        cleanupEmptyChannels();
    }
}
