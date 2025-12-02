#ifndef CLIENT_HPP
#define CLIENT_HPP

#include "Server.hpp"
#include "Channel.hpp"

class Client
{
    private:
        int socketFd;
        bool isPasswordAuthenticated;
        bool isFullyAuthenticated;
        std::string username;
        std::string nickname;
        std::string receiveBuffer;
        std::string ipAddress;
        std::vector<std::string> invitedChannels;
        std::vector<std::string> joinedChannels;
    public:
        Client();
        Client(std::string nickname, std::string username, int socketFd);
        ~Client();
        Client(Client const &src);
        Client &operator=(Client const &src);
        //Getters
        int getSocketFd() const;
        bool getPasswordAuthenticated() const;
        bool getFullyAuthenticated() const;
        bool isInvitedToChannel(std::string &name);
        std::string getUsername() const;
        std::string getNickname() const;
        std::string getBuffer() const;
        const std::vector<std::string>& getJoinedChannels() const;  
        //Setters
        void setSocketFd(int fd);
        void setPasswordAuthenticated(bool value);
        void setFullyAuthenticated(bool value);
        void setUsername(const std::string& username);
        void setNickname(const std::string& nickname);
        void setBuffer(std::string buf);
        void setIpAddress(std::string ipAddress);
        //Utility Methods
        void clearBuffer();
        void addChannelInvites(const std::string& name);
        void removeChannelInvites(const std::string& name);
        void addJoinedChannels(const std::string& name);  
        void removeJoinedChannels(const std::string& name);  
        bool isInChannel(const std::string& name) const;
};

#endif