#include "Server.hpp"
#include <iostream>
#include <cstdlib>
#include <cstring>
#include <cctype>
#include <csignal>

bool isValidPort(const char* portStr)
{
    if (!portStr || strlen(portStr) == 0)
        return false;

    for (size_t i = 0; i < strlen(portStr); ++i)
    {
        if (!isdigit(portStr[i]))
            return false;
    }

    int port = std::atoi(portStr);

    // Must be in valid user-space port range
    if (port < 1024 || port > 65535)
        return false;

    return true;
}

bool isValidPassword(const char* pwd)
{
    if (!pwd || strlen(pwd) == 0)
        return false;

    if (strlen(pwd) > 512) // Max line length in IRC protocol
        return false;

    // Check all characters are printable ASCII
    for (size_t i = 0; i < strlen(pwd); ++i)
    {
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
