#include "Server.hpp"

void Server::PING(const std::string &cmd, int fd)
{
    Client *cli = getClientByFd(fd);
    if (!cli)
        return;
    std::vector<std::string> tokens = splitCommand(cmd);
    if (tokens.size() < 2)
    {
        sendResponse(fd, "409 :No origin specified\r\n");
        return;
    }
    std::string token = tokens[1];
    bool trailing = (token.size() > 0 && token[0] == ':');
    if (!trailing && tokens.size() > 2)
    {
        sendResponse(fd, "461 " + cli->getNickname() + " PING :Too many parameters\r\n");
        return;
    }
    for (size_t i = (trailing ? 1 : 0); i < token.size(); ++i)
    {
        unsigned char c = token[i];
        if (c <= 31 || c == 127)
        {
            sendResponse(fd, "461 " + cli->getNickname() + " PING :Invalid characters in token\r\n");
            return;
        }
    }
    if (!trailing)
        token = ":" + token;
    sendResponse(fd, "PONG " + token + "\r\n");
}
