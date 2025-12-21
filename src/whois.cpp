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

void Server::WHO(const std::string &cmd, int fd)
{
    std::vector<std::string> tokens = splitCommand(cmd);
    Client *cli = getClientByFd(fd);
    if (!cli) return;
    if (tokens.size() < 2)
    {
        sendResponse(fd, ":ircserv 315 " + cli->getNickname() + " :End of WHO list");
        return;
    }

    std::string target = tokens[1];
    std::string requesterNick = cli->getNickname();

    if (target[0] == '#')
    {
        std::string internalName = target.substr(1);
        Channel *ch = getChannel(internalName);
        if (ch) {
            std::vector<Client*> clients = ch->getClientsList();
            for (size_t i = 0; i < clients.size(); ++i)
            {
                Client* m = clients[i];
                std::string flags = "H"; // H = Here (not away)
                if (ch->isChannelOperator(m->getNickname()))
                    flags += "@";

                // Format: :ircserv 352 <req> <chan> <user> <host> <server> <nick> <flags> :0 <realname>
                std::string msg = ":ircserv 352 " + requesterNick + " " + target + " " + 
                  m->getUsername() + " " + m->getIpAddress() + " ircserv " + 
                  m->getNickname() + " " + flags + " :0 " + m->getRealname() + "\r\n";
                sendResponse(fd, msg);
            }
        }
    }
    sendResponse(fd, ":ircserv 315 " + requesterNick + " " + target + " :End of WHO list");
}
