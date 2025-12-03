#ifndef SERVER_HPP
#define SERVER_HPP

#include <csignal> //signal()
#include <cerrno>
#include <cstring> //memset(), strlen()
#include <cstdlib> //atoi(), exit()
#include <ctime> //time(), localtime()
#include <cstddef> //size_t

#include <iostream>
#include <vector>
#include <map>
#include <sstream> //stringstream
#include <fstream>

#include <sys/socket.h> //socket(), bind(), listen(), accept(), send(), recv()
#include <sys/types.h>
#include <netinet/in.h> //sockaddr_in, htons()
#include <arpa/inet.h> //inet_ntoa(), inet_addr()
#include <netdb.h>
#include <fcntl.h>
#include <unistd.h>
#include <poll.h>

#include "Client.hpp"
#include "Channel.hpp"

class Client;
class Channel;

class Server
{
    private:
        int port;
        int socketFd;
        static bool receivedSignal;
        std::string password;
        std::vector<struct pollfd> pollfds;
        struct sockaddr_in address;
        Server(Server const &src);
        Server &operator=(Server const &src);
    public:
        Server();
        ~Server();
        //Getters
        int getPort() const;
        int getSocketFd() const;
        std::string getPassword() const;
        //Setters
        void setPort(int port);
        void setSocketFd(int fd);
        void setPassword(const std::string& password);
        void addPollfd(pollfd fd);
        //Utility Methods
        static void signalHandler(int signal);
        void setupServerSocket();
        void serverInit(int port,const std::string& password);
        void acceptClient();
        void receiveClientData(int fd);
        void fatalError(const char* msg);
};

#endif