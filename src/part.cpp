#include "Server.hpp"

void Server::PART(const std::string &cmd, int fd)
{
    Client *client = getClientByFd(fd);
    if (!client)
        return;
    std::vector<std::string> tokens = splitCommand(cmd);
    std::string nickname = client->getNickname();
    if (tokens.size() < 2)
    {
        sendResponse(fd, "461 " + nickname + " PART :Not enough parameters\r\n");
        return;
    }
    std::vector<std::string> channels = splitString(tokens[1], ',');
    std::string partMessage = nickname + " has left the channel";
    if (tokens.size() > 2)
    {
        partMessage = tokens[2];
        if (!partMessage.empty() && partMessage[0] == ':')
            partMessage = partMessage.substr(1);
    }
    for (size_t i = 0; i < channels.size(); ++i)
    {
        std::string chName = channels[i];
        if (!isValidChannelName(chName))
        {
            sendResponse(fd, "403 " + nickname + " " + chName + " :No such channel\r\n");
            continue;
        }
        std::string internalChannelName = chName.substr(1);
        Channel *ch = getChannel(internalChannelName);
        if (!ch)
        {
            sendResponse(fd, "403 " + nickname + " " + chName + " :No such channel\r\n");
            continue;
        }
        if (!ch->isInChannel(nickname))
        {
            sendResponse(fd, "442 " + nickname + " " + chName + " :You're not on that channel\r\n");
            continue;
        }
        std::ostringstream oss;
        oss << ":" << client->getPrefix() << " PART " << chName << " :" << partMessage << "\r\n";
        ch->broadcastMessage(oss.str());
        ch->removeOperatorByFd(fd);
        ch->removeMemberByFd(fd);
        cleanupEmptyChannels();
        if (ch->getOperatorsCount() == 0 && ch->getChannelTotalClientCount() > 0)
        {
            Client* promote = ch->getFirstMember();
            ch->setAsOperator(promote->getNickname());
            // send MODE broadcast
            std::string msg = ":localhost " + nickname + " MODE #" + internalChannelName + " +o " + promote->getNickname() + "\r\n";
            ch->broadcastMessage(msg);
        }
    }
}
