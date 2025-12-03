#include "Server.hpp"
#include <csignal>
#include <iostream>

void signalHandlerWrapper(int sig)
{
    Server::signalHandler(sig);
}

int main()
{
    // Attach Ctrl+C handler
    std::signal(SIGINT, signalHandlerWrapper);

    std::cout << "Starting test server on port 6667..." << std::endl;

    Server server;
    try
    {
        // Initialize the server
        server.serverInit(6667, "testpass");
    }
    catch (const std::exception &e)
    {
        std::cerr << "Server error: " << e.what() << std::endl;
        return 1;
    }

    std::cout << "Server terminated cleanly." << std::endl;
    return 0;
}
