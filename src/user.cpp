#include "Server.hpp"

void Server::USER(const std::string &cmd, int fd)
{
    Client *cli = getClientByFd(fd);
    if (!cli)
        return;
    std::vector<std::string> tokens = splitCommand(cmd);
    if (tokens.size() < 5) // USER <username> <hostname> <servername> :<realname>
    {
        sendResponse(fd, "461 USER :Not enough parameters\r\n");
        return;
    }
    if (!cli->getUsername().empty())
    {
        sendResponse(fd, "462 USER :You may not reregister\r\n");
        return;
    }
    // Extract username
    std::string username = tokens[1];
    if (!username.empty() && username[0] == ':')
        username.erase(username.begin());
    cli->setUsername(username);
}
