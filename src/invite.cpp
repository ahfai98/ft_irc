#include "Server.hpp"

void Server::INVITE(const std::string &cmd, int fd)
{
    Client *inviter = getClientByFd(fd);
    if (!inviter)
        return;
    std::string inviterNick = inviter->getNickname();
    std::vector<std::string> tokens = splitCommand(cmd);
    if (tokens.size() < 3)
    {
        sendResponse(fd, "461 " + inviterNick + " INVITE :Not enough parameters\r\n");
        return;
    }
    std::string targetNick = tokens[1];
    std::string channelName = tokens[2];
    Client *target = getClientByNickname(targetNick);
    if (!target)
    {
        sendResponse(fd, "401 " + inviterNick + " " + targetNick + " :No such nick/channel\r\n");
        return;
    }
    if (channelName.empty() || channelName[0] != '#')
    {
        sendResponse(fd, "403 " + inviterNick + " " + channelName + " :No such channel\r\n");
        return;
    }
    std::string internalChannelName = channelName.substr(1);
    Channel *ch = getChannel(internalChannelName);
    if (!ch)
    {
        sendResponse(fd, "403 " + inviterNick + " " + channelName + " :No such channel\r\n");
        return;
    }
    if (!ch->isInChannel(inviterNick))
    {
        sendResponse(fd, "442 " + inviterNick + " " + channelName + " :You're not on that channel\r\n");
        return;
    }
    if (ch->getInviteMode() && !ch->isChannelOperator(inviterNick))
    {
        sendResponse(fd, "482 " + inviterNick + " " + channelName + " :You're not channel operator\r\n");
        return;
    }
    if (ch->isInChannel(targetNick))
    {
        sendResponse(fd, "443 " + inviterNick + " " + targetNick + " " + channelName + " :is already on channel\r\n");
        return;
    }
    ch->addInvited(targetNick);
    // Notify inviter (RPL)
    sendResponse(fd, "341 " + inviterNick + " " + targetNick + " " + channelName + "\r\n");
    // Notify target
    std::string msg = ":" + inviter->getPrefix() + " INVITE " + targetNick + " " + channelName + "\r\n";
    sendResponse(target->getSocketFd(), msg);
}
