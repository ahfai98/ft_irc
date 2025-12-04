#include "Server.hpp"

void Server::PASS(const std::string &cmd, int fd)
{
    Client *cli = getClientByFd(fd);
    if (!cli)
        return;

    std::vector<std::string> tokens = splitCommand(cmd);
    if (tokens.size() < 2)
    {
        sendResponse(fd, "461 PASS :Not enough parameters");
        return;
    }

    if (cli->getPasswordAuthenticated())
    {
        sendResponse(fd, "462 PASS :You may not reregister");
        return;
    }
    std::string pass = tokens[1];
    if (!pass.empty() && pass[0] == ':')
        pass.erase(pass.begin());

    if (pass != password)
    {
        sendResponse(fd, "464 PASS :Password incorrect");
        return;
    }
    cli->setPasswordAuthenticated(true);
}
