#include "Server.hpp"

void Server::QUIT(const std::string &cmd, int fd)
{
    Client* client = getClientByFd(fd);
    if (!client)
        return;
    std::vector<std::string> tokens = splitCommand(cmd);
    std::string quitMessage = "Client Quit";
    if (tokens.size() > 1)
    {
        quitMessage = tokens[1];
        if (!quitMessage.empty() && quitMessage[0] == ':')
            quitMessage = quitMessage.substr(1);
        else if (tokens.size() > 2)
        {
            for (size_t i = 2; i < tokens.size(); ++i)
                quitMessage += " " + tokens[i];
        }
    }
    std::string prefix = client->getPrefix();
    std::string nickname = client->getNickname();
    std::vector<std::string> joinedChannels = client->getJoinedChannels();
    for (size_t i = 0; i < joinedChannels.size(); ++i)
    {
        Channel* ch = getChannel(joinedChannels[i]);
        if (!ch)
            continue;

        // 3️⃣ Broadcast QUIT message before removing client
        std::ostringstream oss;
        oss << ":" << prefix << " QUIT :" << quitMessage << "\r\n";
        ch->broadcastMessageExcept(oss.str(), fd);

        // 4️⃣ Remove client from channel
        ch->removeMemberByFd(fd);
        ch->removeOperatorByFd(fd);

        // 5️⃣ Promote new operator if needed
        if (ch->getChannelTotalClientCount() > 0 && ch->getOperatorsCount() == 0)
        {
            Client* promote = ch->getFirstMember();
            if (promote)
            {
                ch->setAsOperator(promote->getNickname());
                std::string modeMsg = ":localhost MODE #" + ch->getChannelName() + " +o " + promote->getNickname() + "\r\n";
                ch->broadcastMessage(modeMsg);
            }
        }
        // 6️⃣ Remove channel if empty
        if (ch->getChannelTotalClientCount() == 0)
            removeChannel(ch->getChannelName());
    }

    // 7️⃣ Finally, remove client from server
    std::cout << "Client <" << fd << "> Disconnected" << std::endl;
    removeClient(fd);
}
