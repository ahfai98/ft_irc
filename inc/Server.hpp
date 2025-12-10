#ifndef SERVER_HPP
#define SERVER_HPP

#include <csignal> //signal()
#include <cerrno>
#include <cstring> //memset(), strlen()
#include <cstdlib> //atoi(), exit()
#include <ctime> //time(), localtime()
#include <iomanip> //setfill() and setwidth
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
        typedef void (Server::*CommandHandler)(const std::string &cmd, int fd);
        std::string dateCreated;
        int port;
        int socketFd;
        static bool receivedSignal;
        std::string password;
        std::vector<struct pollfd> pollfds;
        struct sockaddr_in address;
        std::map<int, Client*> clientsByFd;
        std::map<std::string, Client*> clientsByNickname;
        std::vector<std::string> nicknames;
        std::map<std::string, Channel*> channels;
        std::map<std::string, CommandHandler> commandMap;
        Server(Server const &src);
        Server &operator=(Server const &src);
    public:
        Server();
        ~Server();
        //Getters
        int getPort() const;
        int getSocketFd() const;
        std::string getPassword() const;
        std::string getDateCreated() const;

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

        Client *getClientByFd(int fd);
        Client *getClientByNickname(const std::string& nickname);
        Channel *getChannel(const std::string& name);
        void addClient(Client* cli);
        void addChannel(Channel* ch);
        void removeClient(int fd);
        void clearClients();
        void clearChannels();
        void removePollfd(int fd);
        void removeClientFromAllChannels(Client *cli);

        void sendResponse(int fd, const std::string &message);
        std::vector<std::string> splitCommand(const std::string &cmdLine);

        void initCommandMap();

        void parseExecuteCommand(std::string &cmd, int fd);
        //Handlers
        void PASS(const std::string &cmd, int fd);
        void NICK(const std::string &cmd, int fd);
        void USER(const std::string &cmd, int fd);
        void QUIT(const std::string &cmd, int fd);
        void JOIN(const std::string &cmd, int fd);
        void KICK(const std::string &cmd, int fd);
        void TOPIC(const std::string &cmd, int fd);
        void MODE(const std::string &cmd, int fd);
        void PART(const std::string &cmd, int fd);
        void PRIVMSG(const std::string &cmd, int fd);
        void INVITE(const std::string &cmd, int fd);
        void PING(const std::string &cmd, int fd);
        void NOTICE(const std::string &cmd, int fd);
        
        void sendWelcome(Client *cli);

        //NICK command
        bool isValidNickname(const std::string &nickname);

        //MODE command
        bool isValidKey(const std::string &key);
        bool isValidLimit(const std::string &limit);

        //JOIN command
        std::vector<std::string> splitString(const std::string &str, char delim);
        bool isValidChannelName(const std::string &name);

        //TOPIC command
        std::string trim(std::string &str);

        //PART and QUIT command
        void cleanupEmptyChannels();
};

#endif
