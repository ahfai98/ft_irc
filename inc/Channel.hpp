#ifndef CHANNEL_HPP
#define CHANNEL_HPP

#include "Client.hpp"
#include "Server.hpp"

class Server;
class Client;

class Channel
{
	private:
		bool inviteMode;
		bool topicMode;
		bool keyMode;
		bool limitMode;
		std::string channelName;
		int channelLimit;
		std::string topicName;
		std::string topicSetter;
		std::string timeChannelCreated;
		time_t timeTopicCreated;
		std::string channelKey;
		std::vector<Client*> members;
		std::vector<Client*> operators;
		std::vector<std::string> invitedNicknames;
		Channel(Channel const &src);
		Channel &operator=(Channel const &src);
	public:
		Channel();
		~Channel();
		//Setters
		void setInviteMode(bool value);
		void setTopicMode(bool value);
		void setKeyMode(bool value);
		void setLimitMode(bool value);
		void setChannelName(const std::string& name);
		void setChannelLimit(int limit);
		void setTopicName(const std::string& topic);
		void setTopicSetter(const std::string& setter);
		void setTimeChannelCreated();
		void setTimeTopicCreated();
		void setChannelKey(const std::string& key);
		//Getters
		bool getInviteMode() const;
		bool getTopicMode() const;
		bool getKeyMode() const;
		bool getLimitMode() const;
		const std::string& getChannelName() const;
		int getChannelLimit() const;
		int getChannelTotalClientCount() const;
		const std::string& getTopicName() const;
		const std::string& getTopicSetter() const;
		const std::string& getTimeChannelCreated() const;
		const time_t& getTimeTopicCreated() const;
		const std::string& getChannelKey() const;

		bool isChannelOperator(const std::string& nickname) const;
		bool isChannelMember(const std::string& nickname) const;
		bool isInChannel(const std::string& nickname) const;

		std::string getNamesList() const;

		Client* getClientByFd(std::vector<Client*>& vec, int fd);
		Client* getClientByNickname(std::vector<Client*>& vec, const std::string& nickname);
		Client *getMemberByFd(int fd);
		Client *getOperatorByFd(int fd);
		Client *getMemberByNickname(const std::string& nickname);
		Client *getFirstMember();
		Client *getOperatorByNickname(const std::string& nickname);
		int 	getOperatorsCount() const;
		std::vector<Client*> getClientsList(); 

		//Utility Methods
		void addMember(Client* c);
		void addOperator(Client* c);
		void removeMemberByFd(int fd);
		void removeOperatorByFd(int fd);
		bool setAsOperator(const std::string& nickname);
		bool setAsMember(const std::string& nickname);
		void broadcastMessage(Server& server, const std::string& msg);
		void broadcastMessageExcept(Server& server, const std::string& msg, int fd);
		void broadcastMessageToMembers(Server& server, const std::string& msg);
		void broadcastMessageToOperators(Server& server, const std::string& msg);
		void sendMessageToClients(Server& server, const std::vector<Client*>& clients, const std::string& msg, int excludeFd = -1);
		void addInvited(const std::string& nickname);
		void removeInvited(const std::string& nickname);
		bool isInvited(const std::string& nickname) const;
};

#endif
