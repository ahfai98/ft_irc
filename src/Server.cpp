#include "Server.hpp"

bool Server::receivedSignal = false;

Server::Server()
: port(-1),
  socketFd(-1),
  password("")
{
}

Server::~Server()
{
	clearClients();
    clearChannels();
}

int Server::getPort() const { return port; }
int Server::getSocketFd() const { return socketFd; }
std::string Server::getPassword() const { return password; }

void Server::setPort(int port) { this->port = port; }
void Server::setSocketFd(int fd) { socketFd = fd; }
void Server::setPassword(const std::string &password) { this->password = password; }

void Server::addPollfd(struct pollfd p)
{
	pollfds.push_back(p);
}

void Server::signalHandler(int)
{
	std::cout << "Signal Received!" << std::endl;
	receivedSignal = true;
}

void Server::fatalError(const char *msg)
{
	perror(msg);
	if (socketFd > 0)
		close(socketFd);
	exit(EXIT_FAILURE);
}

void Server::setupServerSocket()
{
	int opt = 1;

	std::memset(&address, 0, sizeof(address));
	address.sin_family = AF_INET;
	address.sin_addr.s_addr = INADDR_ANY;
	address.sin_port = htons(port);

	socketFd = socket(AF_INET, SOCK_STREAM, 0);
	if (socketFd < 0)
		fatalError("socket");

	if (setsockopt(socketFd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
		fatalError("setsockopt");

	if (fcntl(socketFd, F_SETFL, O_NONBLOCK) < 0)
		fatalError("fcntl");

	if (bind(socketFd, (struct sockaddr*)&address, sizeof(address)) < 0)
		fatalError("bind");

	if (listen(socketFd, SOMAXCONN) < 0)
		fatalError("listen");

	struct pollfd p;
	p.fd = socketFd;
	p.events = POLLIN;
	p.revents = 0;
	pollfds.push_back(p);
}

void Server::serverInit(int port, const std::string &password)
{
	this->port = port;
	this->password = password;

	setupServerSocket();

	std::cout << "Server <" << socketFd << "> Connected." << std::endl;
	std::cout << "Waiting to accept a connection..." << std::endl;

	while (!receivedSignal)
	{
		int ret = poll(&pollfds[0], pollfds.size(), -1);

		if (ret == -1)
		{
			if (receivedSignal)
				break;
			perror("poll");
			throw std::runtime_error("poll failed");
		}

		for (size_t i = 0; i < pollfds.size(); ++i)
		{
			if (pollfds[i].revents & POLLIN)
			{
				if (pollfds[i].fd == socketFd)
					acceptClient();
				else
					receiveClientData(pollfds[i].fd);

				// Reset flags for next poll
				pollfds[i].revents = 0;
			}
		}
	}
	std::cout << "Closing Server..." << std::endl;
	clearClients();
	clearChannels();
	close(socketFd);
}

void Server::acceptClient()
{
    struct sockaddr_in clientAddr;
    socklen_t addrlen = sizeof(clientAddr);

    int clientFd = accept(socketFd, (struct sockaddr*)&clientAddr, &addrlen);
    if (clientFd == -1)
    {
        if (errno != EWOULDBLOCK)
            perror("accept");
        return;
    }
    if (fcntl(clientFd, F_SETFL, O_NONBLOCK) == -1)
    {
        perror("fcntl");
        close(clientFd);
        return;
    }
    Client* cli = new Client(clientFd);
    cli->setIpAddress(inet_ntoa(clientAddr.sin_addr));
    addClient(cli);
    struct pollfd p;
    p.fd = clientFd;
    p.events = POLLIN;
    p.revents = 0;
    pollfds.push_back(p);

    std::cout << "Client <" << clientFd << "> Connected." << std::endl;
}

void Server::receiveClientData(int fd)
{
	char buffer[512];
	ssize_t bytes = recv(fd, buffer, sizeof(buffer), 0);

	if (bytes <= 0)
	{
		std::cout << "Client disconnected: fd=" << fd << std::endl;

		// Remove fd from pollfds
		for (size_t i = 0; i < pollfds.size(); ++i)
		{
			if (pollfds[i].fd == fd)
			{
				close(fd);
				pollfds.erase(pollfds.begin() + i);
				break;
			}
		}
		return;
	}

	buffer[bytes] = '\0';
	std::cout << "Received from fd=" << fd << " : " << buffer << std::endl;
}

Client* Server::getClientByFd(int fd)
{
    std::map<int, Client*>::iterator it = clientsByFd.find(fd);
    if (it != clientsByFd.end())
        return it->second;
    return NULL;
}

Client* Server::getClientByNickname(const std::string& nickname)
{
    std::map<std::string, Client*>::iterator it = clientsByNickname.find(nickname);
    if (it != clientsByNickname.end())
        return it->second;
    return NULL;
}

Channel* Server::getChannel(const std::string& name)
{
    std::map<std::string, Channel*>::iterator it = channels.find(name);
    if (it != channels.end())
        return it->second;
    return NULL;
}

// Add / Remove
void Server::addClient(Client* cli)
{
    if (!cli)
		return;
    clientsByFd[cli->getSocketFd()] = cli;
}

void Server::removeClient(int fd)
{
    Client* c = getClientByFd(fd);
    if (!c)
		return;
	removeClientFromAllChannels(c);
    clientsByNickname.erase(c->getNickname());
    clientsByFd.erase(fd);
	close(fd);
    delete c;
}

void Server::removeClientFromAllChannels(Client *cli)
{
	std::vector<std::string> joined = cli->getJoinedChannels();
	std::vector<std::string>::iterator it;
	for (it = joined.begin(); it != joined.end(); ++it)
	{
		Channel* ch = getChannel(*it);
		ch->removeMemberByFd(cli->getSocketFd());
		ch->removeOperatorByFd(cli->getSocketFd());
		if (ch->getChannelTotalClientCount() == 0)
			removeChannel(ch->getChannelName());
	}
    std::map<std::string, Channel*>::iterator it1;
    for (it1 = channels.begin(); it1 != channels.end(); ++it1)
	{
		if (it1->second->isInvited(cli->getNickname()))
		{
			it1->second->removeInvited(cli->getNickname());
			continue;
		}
	}
}

void Server::addChannel(Channel* ch)
{
    if (!ch)
		return;
    channels[ch->getChannelName()] = ch;
}

void Server::removeChannel(const std::string& name)
{
    Channel* c = getChannel(name);
    if (!c)
		return;
    channels.erase(name);
    delete c;
}

// Utility
void Server::clearClients()
{
    std::map<int, Client*>::iterator it;
    for (it = clientsByFd.begin(); it != clientsByFd.end(); ++it)
	{
		close(it->second->getSocketFd());
        delete it->second;
	}
    clientsByFd.clear();
    clientsByNickname.clear();
}

void Server::clearChannels()
{
    std::map<std::string, Channel*>::iterator it;
    for (it = channels.begin(); it != channels.end(); ++it)
        delete it->second;
    channels.clear();
}

void Server::removePollfd(int fd)
{
	std::vector<struct pollfd>::iterator it;
    for (it = pollfds.begin(); it != pollfds.end(); ++it)
	{
		if (it->fd == fd)
		{
        	pollfds.erase(it);
			return;
		}
	}
}

void Server::sendResponse(int fd, const std::string &message)
{
    if (send(fd, message.c_str(), message.size(), 0) == -1)
        std::cerr << "Failed to send to fd " << fd << std::endl;
}

void Server::sendClientError(int fd, int code, const std::string &clientName, const std::string &message)
{
    std::stringstream ss;
    ss << ":localhost " << code << " " << clientName << " " << message;
    sendResponse(fd, ss.str());
}

// Sends an error message to a client for a specific channel
void Server::sendChannelError(int fd, int code, const std::string &clientName,
                              const std::string &channelName, const std::string &message)
{
    std::stringstream ss;
    ss << ":localhost " << code << " " << clientName << " " << channelName << " " << message;
    sendResponse(fd, ss.str());
}
