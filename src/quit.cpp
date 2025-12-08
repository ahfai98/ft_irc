#include "Server.hpp"

void Server::QUIT(const std::string &cmd, int fd)
{
    Client *client = getClientByFd(fd);
    if (!client)
        return;
    std::vector<std::string> tokens = splitCommand(cmd);
    std::string quitMessage = "Client Quit";
    // Handle optional trailing quit message
    if (tokens.size() > 1)
    {
        // Trailing parameter handling
        quitMessage = tokens[1];
        if (!quitMessage.empty() && quitMessage[0] == ':')
            quitMessage = quitMessage.substr(1); // Remove leading ':'
        // If no leading ':', join remaining tokens as message (optional)
        else if (tokens.size() > 2)
        {
            for (size_t i = 2; i < tokens.size(); ++i)
                quitMessage += " " + tokens[i];
        }
    }
    std::string prefix = client->getPrefix();
    // Broadcast QUIT message to all channels the client is in
    std::vector<std::string> channelsCopy = client->getJoinedChannels();
    for (size_t i = 0; i < channelsCopy.size(); ++i)
    {
        Channel *ch = getChannel(channelsCopy[i]);
        if (!ch)
            continue;
        std::ostringstream oss;
        oss << ":" << prefix << " QUIT :" << quitMessage << "\r\n";
        ch->broadcastMessage(oss.str());
        if (ch->getChannelTotalClientCount() == 1)
            continue;
        else if (ch->isChannelOperator(client->getNickname()) && ch->getOperatorsCount() == 1)
        {
            Client* promote = ch->getFirstMember();
            ch->setAsOperator(promote->getNickname());
            // send MODE broadcast
            std::string msg = ":localhost " + client->getNickname() + " MODE #" + ch->getChannelName() + " +o " + promote->getNickname() + "\r\n";
            ch->broadcastMessage(msg);
        }
    }
    removeClient(fd);
}
