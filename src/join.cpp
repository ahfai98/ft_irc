#include "Server.hpp"

void Server::JOIN(const std::string &cmd, int fd)
{
    std::cout << "NICK command from " << fd << std::endl;
	std::cout << cmd << std::endl;
}