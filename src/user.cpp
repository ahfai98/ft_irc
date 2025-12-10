#include "Server.hpp"

static bool isValidUsername(const std::string &username)
{
    if (username.empty())
        return false;
    if (username.size() > 32)
        return false;
    if (username[0] == ':')
        return false;
    const std::string allowedSpecial = "_-[]\\`^{}|";
    for (std::string::size_type i = 0; i < username.size(); ++i)
    {
        char c = username[i];
        if (std::isalnum(c))
            continue;
        if (allowedSpecial.find(c) != std::string::npos)
            continue;
        if (c <= 32 || c == 127)
            return false;
        return false;
    }
    return true;
}

void Server::USER(const std::string &cmd, int fd)
{
    Client *cli = getClientByFd(fd);
    if (!cli)
        return;
    if (cli->getRegistered())
    {
        sendResponse(fd, ":ircserv 462 " + cli->getNickname() + " USER :You may not reregister\r\n");
        return;
    }
    std::vector<std::string> tokens = splitCommand(cmd);
    if (tokens.size() < 5) // USER <username> <hostname> <servername> :<realname>
    {
        sendResponse(fd, ":ircserv 461 " + cli->getNickname() + " USER :Not enough parameters\r\n");
        return;
    }
    if (!cli->getUsername().empty())
    {
        sendResponse(fd, ":ircserv 462 " + cli->getNickname() + " USER :You may not reregister\r\n");
        return;
    }
    std::string username = tokens[1];
    std::string hostname = tokens[2];
    std::string servername = tokens[3];
    //std::string realname = tokens[4];  not used
    if (!isValidUsername(username))
    {
        sendResponse(fd, ":ircserv 461 " + cli->getNickname() + " USER :Username cannot be empty\r\n");
        return;
    }
    if (hostname != "0" || servername != "*")
    {
        sendResponse(fd, ":ircserv 461 " + cli->getNickname() + " USER :Invalid parameters, expected 0 and *\r\n");
        return;
    }
    cli->setUsername(username);
}
