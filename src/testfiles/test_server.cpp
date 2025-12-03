#include "Server.hpp"
#include <iostream>
#include <pthread.h>
#include <unistd.h>
#include <signal.h>
#include <string>
#include <vector>
#include <sys/socket.h>
#include <arpa/inet.h>

Server* g_server = NULL;

// ================================
// Helper for converting int → string (C++98 version)
// ================================
std::string intToString(int n)
{
    std::stringstream ss;
    ss << n;
    return ss.str();
}

// ================================
// Test Client Thread (C++98 compatible)
// ================================
void* testClientThread(void* arg)
{
    int clientId = *(int*)arg;
    sleep(1); // Wait for server to start

    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0)
    {
        perror("client socket");
        return NULL;
    }

    struct sockaddr_in serv;
    serv.sin_family = AF_INET;
    serv.sin_port = htons(6667);
    inet_pton(AF_INET, "127.0.0.1", &serv.sin_addr);

    if (connect(sock, (struct sockaddr*)&serv, sizeof(serv)) < 0)
    {
        perror("client connect");
        close(sock);
        return NULL;
    }

    std::cout << "[Client " << clientId << "] Connected to server" << std::endl;

    // Send some messages
    std::string msg = "Hello from client ";
    msg += intToString(clientId);
    msg += "\r\n";
    send(sock, msg.c_str(), msg.size(), 0);

    sleep(1);

    std::string msg2 = "PING TEST\r\n";
    send(sock, msg2.c_str(), msg2.size(), 0);

    sleep(1);

    close(sock);
    std::cout << "[Client " << clientId << "] Disconnected" << std::endl;
    return NULL;
}

// ================================
// Signal handler
// ================================
void handleSignal(int)
{
    if (g_server)
        g_server->signalHandler(1);
}

// ================================
// Server Thread (no lambda, C++98)
// ================================
void* serverThreadFunc(void*)
{
    try
    {
        g_server->serverInit(6669, "testpass");
    }
    catch (...)
    {
        std::cerr << "Server crashed!" << std::endl;
    }
    return NULL;
}

// ================================
// Main Test Program
// ================================
int main()
{
    struct sigaction sa;
    sa.sa_handler = handleSignal;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;   // disables SA_RESTART so poll() is interrupted
    sigaction(SIGINT, &sa, NULL);

    g_server = new Server();

    pthread_t serverThread;
    pthread_create(&serverThread, NULL, serverThreadFunc, NULL);

    sleep(1); // Allow the server to start

    // Launch test clients
    pthread_t t1, t2;
    int id1 = 1;
    int id2 = 2;

    pthread_create(&t1, NULL, testClientThread, &id1);
    pthread_create(&t2, NULL, testClientThread, &id2);

    pthread_join(t1, NULL);
    pthread_join(t2, NULL);

    // Stop server
    std::cout << "[TEST] Sending SIGINT to stop server" << std::endl;
    handleSignal(0);

    pthread_join(serverThread, NULL);

    delete g_server;

    std::cout << "\n===== TEST COMPLETED SUCCESSFULLY =====" << std::endl;
    return 0;
}
