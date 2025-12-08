#include "Server.hpp"

bool Server::isValidNickname(const std::string &nickname)
{
    if (nickname.empty() || nickname.size() > 9)
        return false;

    const std::string specialFirst = "[]\\`^{}";

    if (!std::isalpha(nickname[0]) && specialFirst.find(nickname[0]) == std::string::npos)
        return false;

    const std::string specialChars = "-[]\\`^{}";

    for (std::string::const_iterator it = nickname.begin() + 1; it != nickname.end(); ++it)
    {
        if (!std::isalnum(*it) && specialChars.find(*it) == std::string::npos)
            return false;
    }
    return true;
}

void Server::NICK(const std::string &cmd, int fd)
{
    Client *cli = getClientByFd(fd);
    if (!cli)
        return;
    std::vector<std::string> tokens = splitCommand(cmd);
    if (tokens.size() < 2)
    {
        sendResponse(fd, "431 * :No nickname given");
        return;
    }
    std::string newNickname = tokens[1];
    if (!newNickname.empty() && newNickname[0] == ':')
        newNickname.erase(newNickname.begin());
    if (!isValidNickname(newNickname))
    {
        sendResponse(fd, "432 " + newNickname + " :Erroneous nickname");
        return;
    }
    if (cli->getNickname() == newNickname)
        return ;
    // Check for nickname collision
    std::map<std::string, Client*>::iterator it = clientsByNickname.find(newNickname);
    if (it != clientsByNickname.end() && it->second != cli)
    {
            sendResponse(fd, "433 * " + newNickname + " :Nickname is already in use");
            return;
    }
    // Remove old nickname from map if exists
    std::string oldNickname = cli->getNickname();
    if (oldNickname != "*" && clientsByNickname[oldNickname] == cli)
        clientsByNickname.erase(oldNickname);

    cli->setNickname(newNickname);
    clientsByNickname[newNickname] = cli;
    std::string oldMask = oldNickname == "*" ? "*" : oldNickname + "!" + cli->getUsername() + "@localhost";
    // Broadcast only if registered
    if (cli->getRegistered())
    {
        for (std::map<std::string, Channel*>::iterator cit = channels.begin(); cit != channels.end(); ++cit)
        {
            Channel *ch = cit->second;
            if (ch->isInvited(oldNickname))
            {
                ch->removeInvited(oldNickname);
                ch->addInvited(newNickname);
            }
            if (ch->isInChannel(newNickname))
                ch->broadcastMessageExcept(":" + oldMask + " NICK :" + newNickname + "\r\n", fd);
        }
    }
    sendResponse(fd, ":" + oldMask + " NICK :" + newNickname + "\r\n");
}
