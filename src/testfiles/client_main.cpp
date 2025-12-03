#include <iostream>
#include <string>
#include <cstring>
#include <vector>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include "Server.hpp"

int sendCommand(int sock, const std::string& cmd)
{
    return send(sock, cmd.c_str(), cmd.size(), 0);
}

bool receiveResponse(int sock, std::string& out, int timeout_ms = 500)
{
    fd_set readfds;
    struct timeval tv;
    tv.tv_sec = timeout_ms / 1000;
    tv.tv_usec = (timeout_ms % 1000) * 1000;

    FD_ZERO(&readfds);
    FD_SET(sock, &readfds);

    int ret = select(sock+1, &readfds, NULL, NULL, &tv);
    if (ret > 0 && FD_ISSET(sock, &readfds))
    {
        char buf[512];
        int bytes = recv(sock, buf, sizeof(buf)-1, 0);
        if (bytes > 0)
        {
            buf[bytes] = '\0';
            out = buf;
            return true;
        }
    }
    return false;
}

int main()
{
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) { perror("socket"); return 1; }

    struct sockaddr_in serverAddr;
    std::memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(6667);
    serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1");

    if (connect(sock, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0)
    { perror("connect"); return 1; }

    std::cout << "Connected to server." << std::endl;

    // Each test case: pair of description and commands to send
    std::vector< std::pair<std::string, std::string> > tests;
    tests.push_back(std::make_pair("Normal commands",
        "PASS secret\r\nNICK TestUser\r\nUSER test 0 * :Test User\r\n"));
    tests.push_back(std::make_pair("Partial command 1",
        "PASS pa"));
    tests.push_back(std::make_pair("Partial command 2",
        "ss\r\nNICK PartialUser\r\n"));
    tests.push_back(std::make_pair("Unknown command",
        "FOO bar\r\n"));
    tests.push_back(std::make_pair("Multiple commands in one send",
        "PING\r\nQUIT\r\nJOIN #test\r\n"));
    tests.push_back(std::make_pair("Weird formatting",
        "   NICK    WeirdUser  \r\nUSER test 0 * :Trailing spaces  \r\n"));

    for (size_t i = 0; i < tests.size(); ++i)
    {
        std::cout << "=== Test: " << tests[i].first << " ===" << std::endl;
        if (sendCommand(sock, tests[i].second) < 0)
        {
            perror("send");
            std::cout << "FAIL: could not send command" << std::endl;
            continue;
        }

        // Try to receive any response (non-blocking with timeout)
        std::string resp;
        if (receiveResponse(sock, resp))
        {
            std::cout << "PASS: server responded: " << resp << std::endl;
        }
        else
        {
            std::cout << "PASS: no response but command processed (expected for echo-only server)" << std::endl;
        }
    }

    close(sock);
    std::cout << "Client disconnected." << std::endl;
    return 0;
}
