#include "Server.hpp"
#include <iostream>
#include <csignal>
#include <unistd.h> // for close()

int main()
{
    Server server;

    server.setPort(6667);
    server.setPassword("secret");

    // Catch SIGINT (Ctrl+C) to stop server
    signal(SIGINT, Server::signalHandler);

    std::cout << "Starting IRC server on port 6667..." << std::endl;
    server.serverInit(6667, "secret");

    std::cout << "Server stopped." << std::endl;
    return 0;
}
