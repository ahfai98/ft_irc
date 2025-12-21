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
    commandMap["WHOIS"]= &Server::WHOIS;
    commandMap["WHO"]= &Server::WHO;
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
    if (!clientsByFd.empty())
        clearClients();
    if (!channels.empty())
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

		for (ssize_t i = pollfds.size() - 1; i >= 0; --i)
		{
			if (pollfds[i].revents & POLLIN)
			{
				if (pollfds[i].fd == socketFd)
					acceptClient();
				else
					receiveClientData(pollfds[i].fd);
            }
            if (pollfds[i].revents & POLLOUT)
                handleClientWrite(pollfds[i].fd);

            pollfds[i].revents = 0;
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
    Client* cli = getClientByFd(fd);
    if (!cli)
        return;

    char buff[512];
    std::memset(buff, 0, sizeof(buff));
    ssize_t bytes = recv(fd, buff, sizeof(buff) - 1, 0);

    if (bytes <= 0)
    {
        std::cout << "Client disconnected: fd=" << fd << std::endl;
        removeClient(fd);
        return;
    }

    // Append raw bytes to the client's internal buffer
    cli->getReceiveBuffer().append(buff, bytes);

    // Process all complete commands found in the buffer
    std::string &buffer = cli->getReceiveBuffer();
    size_t pos;
    while ((pos = buffer.find_first_of("\r\n")) != std::string::npos)
    {
        // Extract the command
        std::string command = buffer.substr(0, pos);
        
        // Find the start of the next command by skipping ALL trailing \r and \n
        size_t next_cmd = buffer.find_first_not_of("\r\n", pos);

        // Execute only if not an empty line
        if (!command.empty())
            parseExecuteCommand(command, fd);

        // Check if client was removed by the command (e.g., QUIT)
        if (!getClientByFd(fd))
            return;

        // Erase the processed command and the delimiters
        if (next_cmd == std::string::npos) {
            buffer.clear();
            break;
        }
        buffer.erase(0, next_cmd);
    }
}

std::vector<std::string> Server::splitCommand(const std::string &cmdLine)
{
    std::vector<std::string> tokens;
    if (cmdLine.empty())
        return tokens;

    std::istringstream iss(cmdLine);
    std::string token;

    while (iss >> token)
    {
        // Handle IRC trailing parameter (the part after the colon)
        if (!token.empty() && token[0] == ':')
        {
            std::string trailing;
            std::getline(iss, trailing);
            
            // The result is the rest of the first word (minus the colon) 
            // plus the rest of the line from getline
            std::string fullTrailing = "";
            if (token.size() > 1)
                fullTrailing = token.substr(1);
            fullTrailing += trailing;
            
            tokens.push_back(fullTrailing);
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

    //Debug
    for (size_t i = 0; i < tokens.size(); ++i)
        std::cout << tokens[i] << std::endl;
    // Case-insensitive command
    std::string command = tokens[0];
    for (size_t i = 0; i < command.size(); ++i)
        command[i] = toupper(command[i]);

    if (command == "CAP")
    {
        if (tokens.size() < 2)
            return;

        std::string subCommand = tokens[1];
        for (size_t i = 0; i < subCommand.size(); ++i)
            subCommand[i] = toupper(subCommand[i]);

        if (subCommand == "LS")
        {
            // Tell the client we support no extra capabilities
            sendResponse(fd, ":ircserv CAP * LS :\r\n");
        }
        else if (subCommand == "END")
        {
            // Negotiation finished. The client will now proceed with NICK/USER
            std::cout << "CAP negotiation ended for fd: " << fd << std::endl;
        }
        return;
    }
    CommandHandler handler = NULL;
    std::map<std::string, CommandHandler>::iterator it = commandMap.find(command);
    if (it != commandMap.end())
    {
        handler = it->second;
        std::cout << " Handler found :" << it->first << std::endl;
    }
    else
        std::cout << " Handler not found :" << command << std::endl;
    Client* cli = getClientByFd(fd);
    if (!cli)
        return;

    if (handler)
    {
        // If client is not registered
        if (!cli->getRegistered())
        {
            //Enforce PASS first if server password is set
            if (!password.empty() && !cli->getPasswordAuthenticated() && command != "PASS")
            {
                sendResponse(fd, ":ircserv 464 * :Password required first\r\n");
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
                printChannelsState();
            }
            else
                sendResponse(fd, ":ircserv 451 * :You have not registered\r\n");
        }
        else
        {
            (this->*handler)(cmd, fd);
            printChannelsState();
        }
    }
    else
        sendResponse(fd, ":ircserv " + cli->getNickname() + " " + command + " :Unknown command\r\n");
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
    // Remove pollfd first to prevent poll events
    removePollfd(fd);
    // Remove client from all channels safely
    removeClientFromAllChannels(c);
    // Erase nickname before deletion to avoid dangling map
    clientsByNickname.erase(c->getNickname());
    clientsByFd.erase(fd);
    // Close socket and delete client object
    close(fd);
    delete c;
    // Cleanup channels with zero clients
    cleanupEmptyChannels();
}

void Server::removeClientFromAllChannels(Client *cli)
{
    if (!cli)
        return;

    for (std::map<std::string, Channel *>::iterator it = channels.begin(); it != channels.end(); ++it)
    {
        Channel* ch = it->second;
        // Remove client from members and operators
        if (it->second->isInvited(cli->getNickname()))
            it->second->removeInvited(cli->getNickname());
        ch->removeMemberByFd(cli->getSocketFd());
        ch->removeOperatorByFd(cli->getSocketFd());
    }
}

void Server::addChannel(Channel* ch)
{
    if (!ch)
		return;
    channels[ch->getChannelName()] = ch;
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
    Client* cli = getClientByFd(fd);
    if (!cli)
        return;

    std::string msg = message;
    if (msg.size() < 2 || msg.substr(msg.size() - 2) != "\r\n")
        msg += "\r\n";

    cli->getSendBuffer() += msg;

    // Enable POLLOUT
    for (size_t i = 0; i < pollfds.size(); ++i)
    {
        if (pollfds[i].fd == fd)
        {
            pollfds[i].events |= POLLOUT;
            break;
        }
    }
}

void Server::handleClientWrite(int fd)
{
    Client* cli = getClientByFd(fd);
    if (!cli)
        return;

    std::string& buffer = cli->getSendBuffer();
    if (buffer.empty())
        return;

    ssize_t sent = send(fd, buffer.c_str(), buffer.size(), 0);

    if (sent > 0)
        buffer.erase(0, sent);
    // Stop watching POLLOUT if nothing left to send
    if (buffer.empty())
    {
        for (size_t i = 0; i < pollfds.size(); ++i)
        {
            if (pollfds[i].fd == fd)
            {
                pollfds[i].events &= ~POLLOUT;
                break;
            }
        }
        if (cli->getQuitting())
        {
            shutdown(fd, SHUT_RDWR);
            removeClient(fd);
        }
    }
}

void Server::sendWelcome(Client *cli)
{
    int fd = cli->getSocketFd();
    std::string nick = cli->getNickname();

    sendResponse(fd, ":ircserv 001 " + nick + " :Welcome to the IRC Network, " + nick + "\r\n");
    sendResponse(fd, ":ircserv 002 " + nick + " :Your host is ircserv, running ft_irc" + "\r\n");
    sendResponse(fd, ":ircserv 003 " + nick + " :This server was created " + getDateCreated()+ "\r\n");
    sendResponse(fd, ":ircserv 004 " + nick + " ircserv ft_irc o o"+ "\r\n");
    // 005: ISUPPORT (CRITICAL for instant sync)
    // Tells Irssi which channel types and modes are valid
    sendResponse(fd, ":ircserv 005 " + nick + " CHANTYPES=# PREFIX=(o)@ CHANMODES=i,t,k,l :are supported by this server\r\n");
}

void Server::cleanupEmptyChannels()
{
    std::vector<std::string> toErase;

    for (std::map<std::string, Channel*>::iterator it = channels.begin(); it != channels.end(); ++it)
    {
        if (it->second && it->second->getChannelTotalClientCount() == 0)
            toErase.push_back(it->first);
    }
    for (size_t i = 0; i < toErase.size(); ++i)
    {
        Channel* ch = channels[toErase[i]];
        delete ch;
        channels.erase(toErase[i]);
    }
}

void Server::printChannelsState()
{
    std::cout << "----- Channels State -----" << std::endl;
    if (channels.empty())
    {
        std::cout << "(no channels)" << std::endl;
        return ;
    }
    for (std::map<std::string, Channel*>::iterator it = channels.begin(); it != channels.end(); ++it)
    {
        Channel* ch = it->second;
        std::cout << "#" << ch->getChannelName() << std::endl;
        std::cout << ch->getNamesList() << std::endl;
    }
    std::cout << "--------------------------" << std::endl;
}