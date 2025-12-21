#include "Server.hpp"

void Server::PASS(const std::string &cmd, int fd)
{
    Client *cli = getClientByFd(fd);
    if (!cli)
        return;

    std::vector<std::string> tokens = splitCommand(cmd);
    if (tokens.size() < 2)
    {
        sendResponse(fd, ":ircserv 461 * PASS :Not enough parameters\r\n");
        return;
    }

    if (cli->getRegistered() || cli->getPasswordAuthenticated())
    {
        sendResponse(fd, ":ircserv 462 * PASS :You may not reregister\r\n");
        return;
    }
    std::string pass = tokens[1];
    if (!pass.empty() && pass[0] == ':')
    {
        pass.erase(pass.begin());
        pass = trim(pass);
    }
    if (pass != password)
    {
        cli->setPasswordAuthenticated(false);
        sendResponse(fd, ":ircserv 464 * PASS :Password incorrect\r\n");
        return;
    }
    cli->setPasswordAuthenticated(true);
}
