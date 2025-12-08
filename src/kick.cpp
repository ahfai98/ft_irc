#include "Server.hpp"

void Server::KICK(const std::string &cmd, int fd)
{
    Client *client = getClientByFd(fd);
    if (!client)
        return;
    std::string nickname = client->getNickname();
    std::vector<std::string> tokens = splitCommand(cmd);
    if (tokens.size() < 3)
    {
        sendResponse(fd, "461 " + nickname + " KICK :Not enough parameters\r\n");
        return;
    }
    std::string channelList = tokens[1]; // comma-separated channels
    std::string userList = tokens[2];    // comma-separated users
    std::string reason = "";
    // Trailing reason
    if (tokens.size() > 3)
    {
        reason = tokens[3];
        if (!reason.empty() && reason[0] == ':')
            reason = reason.substr(1); // remove leading ':'
    }
    // Split channels
    std::vector<std::string> channels = splitString(channelList, ',');
    // Split users
    std::vector<std::string> users = splitString(userList, ',');

    // Validate mapping: one user for multiple channels OR same number of users as channels
    if (!(users.size() == 1 || users.size() == channels.size()))
    {
        sendResponse(fd, "461 " + nickname + " KICK :Not enough parameters\r\n");
        return;
    }
    for (size_t i = 0; i < channels.size(); ++i)
    {
        std::string chName = channels[i];
        if (!isValidChannelName(chName))
        {
            sendResponse(fd, "403 " + nickname + " " + chName + " :No such channel\r\n");
            continue;
        }
        std::string internalName = chName.substr(1);
        Channel *ch = getChannel(internalName);
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

        if (!ch->isChannelOperator(nickname))
        {
            sendResponse(fd, "482 " + nickname + " " + chName + " :You're not channel operator\r\n");
            continue;
        }
        std::string targetNick = (users.size() == 1) ? users[0] : users[i];
        if (!ch->isInChannel(targetNick))
        {
            sendResponse(fd, "441 " + nickname + " " + targetNick + " " + chName + " :They aren't on that channel\r\n");
            continue;
        }
        Client *target = getClientByNickname(targetNick);
        if (!target)
        {
            sendResponse(fd, "401 " + nickname + " " + targetNick + " :No such nick\r\n");
            continue;
        }
        if (targetNick == nickname)
        {
            sendResponse(fd, "482 " + nickname + " " + chName + " :You cannot kick yourself\r\n");
            continue;
        }
        // Build KICK message using client prefix
        std::ostringstream oss;
        oss << ":" << client->getPrefix() << " KICK " << chName << " " << targetNick;
        if (!reason.empty())
            oss << " :" << reason;
        oss << "\r\n";
        ch->broadcastMessage(oss.str());

        // Remove target from channel
        if (ch->isChannelMember(targetNick))
            ch->removeMemberByFd(target->getSocketFd());
        else
            ch->removeOperatorByFd(target->getSocketFd());
    }
}
