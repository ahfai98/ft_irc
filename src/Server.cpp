#include "Server.hpp"

bool Server::receivedSignal = false;

void Server::initCommandMap()
{
    commandMap["PASS"]   = &Server::PASS;
    commandMap["NICK"]   = &Server::NICK;
    commandMap["USER"]   = &Server::USER;
    commandMap["QUIT"]   = &Server::QUIT;
    commandMap["JOIN"]   = &Server::JOIN;
    commandMap["KICK"]   = &Server::KICK;
    commandMap["TOPIC"]  = &Server::TOPIC;
    commandMap["MODE"]   = &Server::MODE;
    commandMap["PART"]   = &Server::PART;
    commandMap["PRIVMSG"]= &Server::PRIVMSG;
    commandMap["INVITE"] = &Server::INVITE;
    commandMap["PING"]   = &Server::PING;
    commandMap["NOTICE"]= &Server::NOTICE;
}

Server::Server()
: port(-1),
  socketFd(-1),
  password("")
{
	initCommandMap();
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    std::ostringstream oss;
    oss << std::setw(4) << std::setfill('0') << (t->tm_year + 1900) << "-"
        << std::setw(2) << std::setfill('0') << (t->tm_mon + 1) << "-"
        << std::setw(2) << std::setfill('0') << t->tm_mday;
    dateCreated = oss.str();
}

Server::~Server()
{
	clearClients();
    clearChannels();
}

int Server::getPort() const { return port; }
int Server::getSocketFd() const { return socketFd; }
std::string Server::getPassword() const { return password; }
std::string Server::getDateCreated() const { return dateCreated; }

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
    char buff[512];
    Client* cli = getClientByFd(fd);
    if (!cli)
        return;
    ssize_t bytes = recv(fd, buff, sizeof(buff), 0);

    // Client disconnected or error
    if (bytes <= 0)
    {
        std::cout << "Client disconnected: fd=" << fd << std::endl;
        removeClient(fd);
        return;
    }

    // Append new data to client's buffer
    cli->setBuffer(cli->getBuffer() + std::string(buff, bytes));

    std::string& buf = cli->getBuffer();
    size_t pos;

    // Extract and execute complete commands ending with \r\n
    while ((pos = buf.find("\r\n")) != std::string::npos)
    {
        std::string command = buf.substr(0, pos);

        // Execute the command
        parseExecuteCommand(command, fd);

        // Remove processed command + \r\n from buffer
        buf.erase(0, pos + 2);
    }
}

std::vector<std::string> Server::splitCommand(const std::string &cmdLine)
{
    std::vector<std::string> tokens;
    std::istringstream iss(cmdLine);
    std::string token;

    while (iss >> token)
    {
        if (!token.empty() && token[0] == ':')
        {
            std::string trailing;
            std::getline(iss, trailing);
            tokens.push_back(token + trailing); 
            break;
        }
        tokens.push_back(token);
    }
    return tokens;
}


void Server::parseExecuteCommand(std::string &cmd, int fd)
{
    if (cmd.empty())
        return;

    // Trim leading whitespace
    cmd = trim(cmd);

    std::vector<std::string> tokens = splitCommand(cmd);
    if (tokens.empty())
        return;

    // Case-insensitive command
    std::string command = tokens[0];
    for (size_t i = 0; i < command.size(); ++i)
        command[i] = toupper(command[i]);

    CommandHandler handler = NULL;
    std::map<std::string, CommandHandler>::iterator it = commandMap.find(command);
    if (it != commandMap.end())
        handler = it->second;

    Client* cli = getClientByFd(fd);
    if (!cli)
        return;

    if (handler)
    {
        // If client is not registered
        if (!cli->getRegistered())
        {
            // Enforce PASS first if server password is set
            if (!cli->getPasswordAuthenticated() && command != "PASS")
            {
                sendResponse(fd, "464 :Password required first\r\n");
                return;
            }
            // Only allow PASS/NICK/USER before registration
            if (command == "PASS" || command == "NICK" || command == "USER")
            {
                (this->*handler)(cmd, fd);

                // Try completing registration: PASS must be correct if required
                if (!cli->getRegistered() && cli->getPasswordAuthenticated() &&
                    cli->getNickname() != "*" &&
                    !cli->getUsername().empty())
                {
                    cli->setRegistered(true);
                    sendWelcome(cli);
                }
            }
            else
                sendResponse(fd, "451 :You have not registered\r\n");
        }
        else
        {
            (this->*handler)(cmd, fd);
        }
    }
    else
        sendResponse(fd, "ERR_CMDNOTFOUND");
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

    // 1. Remove client from pollfds first
    removePollfd(fd);

    // 2. Remove client from all channels safely
    removeClientFromAllChannels(c);

    // 3. Remove client from maps
    clientsByNickname.erase(c->getNickname());
    clientsByFd.erase(fd);

    // 4. Close socket before deleting client
    close(fd);

    // 5. Delete client
    delete c;
}


void Server::removeClientFromAllChannels(Client *cli)
{
    if (!cli)
        return;

    // 1. Copy the joined channel names to avoid iterator invalidation
    std::vector<std::string> joined = cli->getJoinedChannels();
    std::vector<std::string> emptyChannels;

    // 2. Remove client from each channel
    for (std::vector<std::string>::iterator it = joined.begin(); it != joined.end(); ++it)
    {
        Channel* ch = getChannel(*it);
        if (!ch)
            continue;

        // Remove client from members and operators
        ch->removeMemberByFd(cli->getSocketFd());
        ch->removeOperatorByFd(cli->getSocketFd());

        // Mark empty channels to delete later
        if (ch->getChannelTotalClientCount() == 0)
            emptyChannels.push_back(*it);
    }

    // 3. Remove empty channels after iteration
    for (std::vector<std::string>::iterator it = emptyChannels.begin(); it != emptyChannels.end(); ++it)
        removeChannel(*it);

    // 4. Remove invitations for this client
    for (std::map<std::string, Channel*>::iterator it = channels.begin(); it != channels.end(); ++it)
    {
        if (it->second->isInvited(cli->getNickname()))
            it->second->removeInvited(cli->getNickname());
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
    Channel* ch = getChannel(name);
    if (!ch)
        return;

    // 1. Remove from map first
    channels.erase(name);

    // 2. Delete the channel object
    delete ch;
}


// Utility
void Server::clearClients()
{
    // Copy client pointers to a temporary vector to safely delete
    std::vector<Client*> tempClients;
    for (std::map<int, Client*>::iterator it = clientsByFd.begin(); it != clientsByFd.end(); ++it)
        tempClients.push_back(it->second);

    // Close all sockets and delete clients
    for (std::vector<Client*>::iterator it = tempClients.begin(); it != tempClients.end(); ++it)
    {
        if (*it)
        {
            close((*it)->getSocketFd());
            delete *it;
        }
    }

    // Clear the maps after deletion
    clientsByFd.clear();
    clientsByNickname.clear();
}


void Server::clearChannels()
{
    // Copy all channel pointers to a temporary vector to safely delete
    std::vector<Channel*> tempChannels;
    for (std::map<std::string, Channel*>::iterator it = channels.begin(); it != channels.end(); ++it)
        tempChannels.push_back(it->second);

    // Delete all channels
    for (std::vector<Channel*>::iterator it = tempChannels.begin(); it != tempChannels.end(); ++it)
    {
        if (*it)
            delete *it;
    }
    // Clear the map after deletion
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
    std::string msgWithCRLF = message;
    if (msgWithCRLF.size() < 2 || msgWithCRLF.substr(msgWithCRLF.size()-2) != "\r\n")
        msgWithCRLF += "\r\n";

    if (send(fd, msgWithCRLF.c_str(), msgWithCRLF.size(), 0) == -1)
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

void Server::sendWelcome(Client *cli)
{
    int fd = cli->getSocketFd();
    std::string nick = cli->getNickname();
    std::string user = cli->getUsername();

    sendResponse(fd, "001 " + nick + " :Welcome to the IRC Network, " + nick);
    sendResponse(fd, "002 " + nick + " :Your host is localhost, running ft_irc");
    sendResponse(fd, "003 " + nick + " :This server was created " + getDateCreated());
    sendResponse(fd, "004 " + nick + " localhost ft_irc o o");
}
