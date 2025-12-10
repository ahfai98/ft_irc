#include "Server.hpp"

void Server::PING(const std::string &cmd, int fd)
{
    Client *cli = getClientByFd(fd);
    if (!cli)
        return;
    std::vector<std::string> tokens = splitCommand(cmd);
    if (tokens.size() < 2)
    {
        sendResponse(fd, ":ircserv 409 " + cli->getNickname() + " :No origin specified\r\n");
        return;
    }
    std::string token = tokens[1];
    // Remove leading colon
    if (!token.empty() && token[0] == ':')
        token.erase(token.begin());
    if (token.empty())
    {
        sendResponse(fd, ":ircserv 461 " + cli->getNickname() + " PING :Not enough parameters\r\n");
        return;
    }
    sendResponse(fd, "PONG :" + token + "\r\n");
}
