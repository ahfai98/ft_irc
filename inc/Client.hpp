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
        //prevent copying
        Client(const Client &src);
        Client &operator=(const Client &src);
    public:
        Client(int fd);
        ~Client();
        //Getters
        int getSocketFd() const;
        bool getPasswordAuthenticated() const;
        bool getFullyAuthenticated() const;
        bool isInvitedToChannel(const std::string &name) const;
        std::string getUsername() const;
        std::string getNickname() const;
        std::string getBuffer() const;
        const std::vector<std::string>& getInvitedChannels() const;  
        const std::vector<std::string>& getJoinedChannels() const;
        //Setters
        void setSocketFd(int fd);
        void setPasswordAuthenticated(bool value);
        void setFullyAuthenticated(bool value);
        void setUsername(const std::string& username);
        void setNickname(const std::string& nickname);
        void setBuffer(const std::string& buf);
        void setIpAddress(const std::string& ipAddress);
        //Utility Methods
        void clearBuffer();
        void addChannelInvites(const std::string& name);
        void removeChannelInvites(const std::string& name);
        void addJoinedChannels(const std::string& name);  
        void removeJoinedChannels(const std::string& name);  
        bool isInChannel(const std::string& name) const;
};

#endif