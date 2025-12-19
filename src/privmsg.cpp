#include "Server.hpp"

void Server::PRIVMSG(const std::string &cmd, int fd)
{
    Client *cli = getClientByFd(fd);
    if (!cli)
        return;
    std::vector<std::string> tokens = splitCommand(cmd);
    std::string nickname = cli->getNickname();
    if (tokens.size() < 2)
    {
        sendResponse(fd, ":ircserv 411 " + nickname + " :No recipient given\r\n");
        return;
    }
    if (tokens.size() < 3)
    {
        sendResponse(fd, ":ircserv 412 " + nickname + " :No text to send\r\n");
        return;
    }
    // Extract targets and message
    std::string targetStr = tokens[1];
    std::string message;
    if (tokens[2][0] != ':')
        message = tokens[2];
    else
    {
        for (size_t i = 2; i < tokens.size(); ++i)
        {
            if (i > 2)
                message += " ";
            message += tokens[i];
        }
    }
    if (!message.empty() && message[0] == ':')
        message = message.substr(1);
    message = trim(message);
    if (message.empty())
    {
        sendResponse(fd, ":ircserv 412 " + nickname + " :No text to send\r\n");
        return;
    }
    std::vector<std::string> targets = splitString(targetStr, ',');
    for (size_t i = 0; i < targets.size(); ++i)
    {
        if (targets[i].empty())
        {
            targets.erase(targets.begin() + i);
            --i;
        }
    }
    if (targets.size() > 10)
    {
        sendResponse(fd, ":ircserv 407 " + nickname + " :Too many recipients\r\n");
        return;
    }
    for (size_t i = 0; i < targets.size(); ++i)
    {
        std::string target = targets[i];
        if (target.empty())
            continue;

        if (target[0] == '#')
        { // Channel
            std::string chName = target.substr(1);
            Channel *ch = getChannel(chName);
            if (!ch)
            {
                sendResponse(fd, ":ircserv 403 " + nickname + " " + target + " :No such channel\r\n");
                continue;
            }
            if (!ch->isInChannel(nickname))
            {
                sendResponse(fd, ":ircserv 404 " + nickname + " " + target + " :Cannot send to channel\r\n");
                continue;
            }
            std::ostringstream oss;
            oss << ":" << cli->getPrefix() << " PRIVMSG " << target << " :" << message << "\r\n";
            ch->broadcastMessageExcept(*this, oss.str(), fd);
        }
        else
        { // User
            Client *targetClient = getClientByNickname(target);
            if (!targetClient)
            {
                sendResponse(fd, ":ircserv 401 " + nickname + " " + target + " :No such nick\r\n");
                continue;
            }
            std::ostringstream oss;
            oss << ":" << cli->getPrefix() << " PRIVMSG " << target << " :" << message << "\r\n";
            sendResponse(targetClient->getSocketFd(), oss.str());
        }
    }
}
