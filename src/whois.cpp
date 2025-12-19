#include "Server.hpp"

void Server::WHOIS(const std::string &cmd, int fd)
{
    std::vector<std::string> tokens = splitCommand(cmd);
    if (tokens.size() < 2)
    {
        sendResponse(fd, ":ircserv 461 * WHOIS :Not enough parameters\r\n");
        return;
    }

    std::string targetNick = tokens[1];
    Client* target = getClientByNickname(targetNick);
    Client* requester = getClientByFd(fd);

    if (!target)
    {
        sendResponse(fd, ":ircserv 401 " + targetNick + " :No such nick\r\n");
        return;
    }
    // Send WHOIS info
    std::string nick = requester->getNickname();
    std::string username = target->getUsername();
    std::string host = target->getIpAddress();
    std::string realName = target->getRealname();

    // 311 = WHOIS reply
    sendResponse(fd, ":ircserv 311 " + nick + " " + targetNick + " " + username + " " + host + " * :" + realName + "\r\n");
    
    // 312 = Server info
    sendResponse(fd, ":ircserv 312 " + nick + " " + targetNick + " ircserv :ft_irc server\r\n");

    // 318 = End of WHOIS
    sendResponse(fd, ":ircserv 318 " + nick + " " + targetNick + " :End of WHOIS list\r\n");
}
