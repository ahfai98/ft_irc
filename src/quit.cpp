#include "Server.hpp"

void Server::QUIT(const std::string& cmd, int fd)
{
    Client* client = getClientByFd(fd);
    if (!client)
        return;
    std::string quitMessage;
    size_t pos = cmd.find(':');
    if (pos != std::string::npos && pos + 1 < cmd.size())
        quitMessage = cmd.substr(pos + 1);
    else
        quitMessage = "Quit";
    std::string prefix = client->getPrefix();
    std::string nickname = client->getNickname();
    std::vector<Channel*> clientChannels;
    for (std::map<std::string, Channel*>::iterator it = channels.begin(); it != channels.end(); ++it)
    {
        if (it->second->isInChannel(nickname))
            clientChannels.push_back(it->second);
    }
    // Broadcast QUIT message to all channels
    for (size_t i = 0; i < clientChannels.size(); ++i)
    {
        std::string msg = ":" + prefix + " QUIT :" + quitMessage + "\r\n";
        clientChannels[i]->broadcastMessage(*this, msg);
    }
    std::string finalMsg = "ERROR :Closing Link: " + quitMessage + "\r\n";
    sendResponse(fd, finalMsg);
    client->setQuitting(true);
}
