#ifndef CLIENT_HPP
#define CLIENT_HPP

#include "Server.hpp"
#include "Channel.hpp"

class Client
{
	private:
		int socketFd;
		bool isPasswordAuthenticated;
		bool isRegistered;
		bool isQuitting;
		std::string username;
		std::string realname;
		std::string nickname;
		std::string receiveBuffer;
		std::string sendBuffer;
		std::string ipAddress;
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
		bool getRegistered() const;
		bool getQuitting() const;
		const std::string& getUsername() const;
		const std::string& getRealname() const;
		const std::string& getNickname() const;
		std::string& getReceiveBuffer();
		std::string& getSendBuffer() ;
		const std::vector<std::string>& getJoinedChannels() const;
		std::string getPrefix() const;
		const std::string& getIpAddress() const;
		//Setters
		void setSocketFd(int fd);
		void setPasswordAuthenticated(bool value);
		void setRegistered(bool value);
		void setQuitting(bool value);
		void setUsername(const std::string& username);
		void setRealname(const std::string& realname);
		void setNickname(const std::string& nickname);
		void setReceiveBuffer(const std::string& buf);
		void setSendBuffer(const std::string& buf);
		void setIpAddress(const std::string& ipAddress);
		//Utility Methods
		void clearReceiveBuffer();
		void clearSendBuffer();
		void addChannelInvites(const std::string& name);
		bool isInChannel(const std::string& name) const;
		void addJoinedChannels(const std::string& name);
		void removeJoinedChannels(const std::string& name);
};

#endif