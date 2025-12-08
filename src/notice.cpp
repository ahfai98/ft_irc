#include "Server.hpp"

void Server::NOTICE(const std::string &cmd, int fd)
{
    Client *client = getClientByFd(fd);
    if (!client)
        return;
    std::vector<std::string> tokens = splitCommand(cmd);
    if (tokens.size() < 2)
        return; // No target, silently ignore
    std::string targetStr = tokens[1];
    std::string message = tokens[2];
    if (!message.empty() && message[0] == ':')
        message = message.substr(1);
    // Trim leading/trailing spaces
    message = trim(message);
    if (message.empty())
        return;
    // Split targets by comma
    std::vector<std::string> targets = splitString(targetStr, ',');
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
                continue;
            if (!ch->isInChannel(client->getNickname()))
                continue;
            std::ostringstream oss;
            oss << ":" << client->getPrefix() << " NOTICE " << target << " :" << message << "\r\n";
            ch->broadcastMessageExcept(oss.str(), fd);
        }
        else
        { // User
            Client *targetClient = getClientByNickname(target);
            if (!targetClient)
                continue;
            std::ostringstream oss;
            oss << ":" << client->getPrefix() << " NOTICE " << target << " :" << message << "\r\n";
            sendResponse(targetClient->getSocketFd(), oss.str());
        }
    }
}
