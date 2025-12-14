#include "Server.hpp"
#include <iostream>
#include <cstdlib>
#include <cstring>
#include <cctype>
#include <csignal>

bool isValidPort(const std::string &portStr)
{
    if (portStr.empty())
        return false;

    unsigned long port = 0;
    for (std::string::size_type i = 0; i < portStr.size(); ++i)
    {
        if (!std::isdigit(portStr[i]))
            return false;
        // Multiply current value by 10 and add digit
        port = port * 10 + (portStr[i] - '0');

        // Check overflow (max port is 65535)
        if (port > 65535)
            return false;
    }
    // Must be in valid user-space port range
    if (port < 1024)
        return false;
    return true;
}

bool isValidPassword(const std::string &pwd)
{
    if (pwd.empty())
        return false;
    if (pwd.length() > 512) //Max IRC line length
        return false;
    for (std::string::size_type i = 0; i < pwd.length(); ++i)
    {
        //Printable ASCII only (32-126)
        if (pwd[i] < 32 || pwd[i] > 126)
            return false;
    }
    return true;
}

int main(int argc, char* argv[])
{
    if (argc != 3)
    {
        std::cerr << "Usage: ./ircserv <port> <password>" << std::endl;
        return 1;
    }

    if (!isValidPort(argv[1]))
    {
        std::cerr << "Error: Invalid port number. Use 1024-65535." << std::endl;
        return 1;
    }

    if (!isValidPassword(argv[2]))
    {
        std::cerr << "Error: Invalid password. Must be 1-512 printable ASCII characters." << std::endl;
        return 1;
    }

    int port = std::atoi(argv[1]);
    std::string password = argv[2];

    Server ircServer;
    try
    {
        // Setup signal handlers
        std::signal(SIGINT, Server::signalHandler);
        std::signal(SIGQUIT, Server::signalHandler);
        std::signal(SIGPIPE, SIG_IGN);  // Ignore broken pipe signals

        std::cout << "Starting IRC server on port " << port << "..." << std::endl;
        ircServer.serverInit(port, password);
    }
    catch (const std::exception& e)
    {
        std::cerr << "Server failed: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}
