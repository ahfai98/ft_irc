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
	commandMap["NOTICE"] = &Server::NOTICE;
	commandMap["WHOIS"]  = &Server::WHOIS;
	commandMap["WHO"]    = &Server::WHO;
}

Server::Server()
: port(-1), socketFd(-1), password("")
{
	initCommandMap();
	time_t now = time(NULL);
	char *dt = ctime(&now);
	std::string str(dt);
	if (!str.empty() && str[str.length() - 1] == '\n')
		str.erase(str.length() - 1);
	dateCreated = str;
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
	if (socketFd != -1)
	{
		close(socketFd);
		socketFd = -1;
	}
	exit(EXIT_FAILURE);
}

void Server::setupServerSocket()
{
	int opt = 1;
	std::memset(&address, 0, sizeof(address));
	address.sin_family = AF_INET; //IPv4
	address.sin_addr.s_addr = INADDR_ANY; //Accept connections from any IP
	address.sin_port = htons(port); //Host-to-Network Short (Endianness)
	//converts to Network Byte Order required by TCP/IP stack

	socketFd = socket(AF_INET, SOCK_STREAM, 0); //SOCK_STREAM specifies TCP
	if (socketFd == -1)
		fatalError("Failed to create socket");

	if (setsockopt(socketFd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1)
		fatalError("Failed to set SO_REUSEADDR");

	if (fcntl(socketFd, F_SETFL, O_NONBLOCK) == -1)
		fatalError("Failed to set non-blocking mode");

	if (bind(socketFd, (struct sockaddr*)&address, sizeof(address)) == -1)
		fatalError("Failed to bind to port");

	if (listen(socketFd, SOMAXCONN) == -1) //Moves socket from active(connect) to passive(wait to connect) state
		fatalError("Failed to listen"); //SOMAXCONN is maximum number of pending connections that can wait in queue before server rjects

	struct pollfd p;
	std::memset(&p, 0, sizeof(p));
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
			continue;
		}
		for (size_t i = 0; i < pollfds.size(); ++i)
		{
			if (pollfds[i].revents & POLLIN)
			{
				if (pollfds[i].fd == socketFd)
					acceptClient();
				else
				{
					int currentFd = pollfds[i].fd;
					receiveClientData(currentFd);
					if (i >= pollfds.size() || pollfds[i].fd != currentFd)
					{
						i--;
						continue;
					}
				}
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
	std::memset(&clientAddr, 0, sizeof(clientAddr));
	socklen_t addrlen = sizeof(clientAddr);
	int clientFd = accept(socketFd, (struct sockaddr*)&clientAddr, &addrlen);
	if (clientFd == -1)
	{
		perror("accept");
		return;
	}
	if (fcntl(clientFd, F_SETFL, O_NONBLOCK) == -1)
	{
		perror("fcntl O_NONBLOCK failed");
		close(clientFd);
		return;
	}
	Client* cli = new Client(clientFd);
	if (!cli)
	{
		std::cerr << "Memory allocation failed for new client" << std::endl;
		close(clientFd);
		return;
	}
	cli->setIpAddress(inet_ntoa(clientAddr.sin_addr)); 
	//convert IPv4 address from binary(network byte order) into string (e.g., "192.168.1.1")
	addClient(cli);
	struct pollfd p;
	std::memset(&p, 0, sizeof(p));
	p.fd = clientFd;
	p.events = POLLIN;
	p.revents = 0;
	pollfds.push_back(p);
	std::cout << "Client <" << clientFd << "> Connected from " << cli->getIpAddress() << std::endl;
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
	buff[bytes] = '\0';
	cli->getReceiveBuffer() += buff;
	//Protect against Buffer Overflow/ Infinite "Partial Messages"
	if (cli->getReceiveBuffer().size() > 5120)
	{ //Safety limit of 10 messages
		std::cerr << "Client <" << fd << "> buffer overflow. Closing." << std::endl;
		removeClient(fd);
		return;
	}
	std::string &buffer = cli->getReceiveBuffer();
	size_t pos;
	while ((pos = buffer.find("\n")) != std::string::npos)
	{
		std::string line = buffer.substr(0, pos);
		//Trim \r if found
		if (!line.empty() && line[line.size() - 1] == '\r')
			line.erase(line.size() - 1);
		buffer.erase(0, pos + 1);
		if (!line.empty())
			parseExecuteCommand(line, cli->getSocketFd());
		//If the client quit, stop processing
		if (!getClientByFd(cli->getSocketFd()))
			return;
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
	if (sent == -1)
	{
		perror("send error");
		removeClient(fd);
		return;
	}
	if (sent > 0)
		buffer.erase(0, sent);
	if (buffer.empty())
	{
		//Turns POLLOUT off after send
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
			std::cout << "Client <" << fd << "> quit gracefully." << std::endl;
			removeClient(fd);
		}
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
		//Find Trailing
		if (!token.empty() && token[0] == ':')
		{
			std::string trailing;
			std::getline(iss, trailing);
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
	//Trim whitespace
	cmd = trim(cmd);
	std::vector<std::string> tokens = splitCommand(cmd);
	if (tokens.empty())
		return;
	//Case-insensitive command
	std::string command = tokens[0];
	for (size_t i = 0; i < command.size(); ++i)
		command[i] = toupper(command[i]);
	//CAP command
	if (command == "CAP") //Capability Negotiation
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
		handler = it->second;
	Client* cli = getClientByFd(fd);
	if (!cli)
		return;
	if (handler)
	{
		if (!cli->getRegistered())
		{
			//Enforce PASS first
			if (!password.empty() && !cli->getPasswordAuthenticated() && command != "PASS")
			{
				sendResponse(fd, ":ircserv 464 * :Password required first\r\n");
				return;
			}
			// Only allow PASS/NICK/USER before registration
			if (command == "PASS" || command == "NICK" || command == "USER")
			{
				(this->*handler)(cmd, fd);

				//Try completing registration
				if (!cli->getRegistered() && cli->getPasswordAuthenticated() &&
					cli->getNickname() != "*" && !cli->getUsername().empty())
				{
					cli->setRegistered(true);
					sendWelcome(cli);
				}
				//printChannelsState();
			}
			else
				sendResponse(fd, ":ircserv 451 * :You have not registered\r\n");
		}
		else
		{
			(this->*handler)(cmd, fd);
			//printChannelsState();
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


void Server::addClient(Client* cli)
{
	if (!cli)
		return;
	clientsByFd[cli->getSocketFd()] = cli;
}

void Server::removeClient(int fd)
{
	Client* cli = getClientByFd(fd);
	if (!cli)
		return;
	removePollfd(fd);
	removeClientFromAllChannels(cli);
	clientsByNickname.erase(cli->getNickname());
	clientsByFd.erase(fd);
	close(fd);
	delete cli;
	cleanupEmptyChannels();
}

void Server::removeClientFromAllChannels(Client *cli)
{
	if (!cli)
		return;
	std::map<std::string, Channel*>::iterator it;
	for (it = channels.begin(); it != channels.end(); ++it)
	{
		Channel* ch = it->second;
		if (ch->isInvited(cli->getNickname()))
			ch->removeInvited(cli->getNickname());
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


void Server::clearClients()
{
	if (clientsByFd.empty())
		return;
	std::vector<Client*> tempClients;
	std::map<int, Client*>::iterator it;
	for (it = clientsByFd.begin(); it != clientsByFd.end(); ++it)
		tempClients.push_back(it->second);
	//Close all sockets and delete clients
	std::vector<Client*>::iterator it1;
	for (it1 = tempClients.begin(); it1 != tempClients.end(); ++it1)
	{
		if (*it1)
		{
			close((*it1)->getSocketFd());
			delete *it1;
		}
	}
	clientsByFd.clear();
	clientsByNickname.clear();
}

void Server::clearChannels()
{
	if (channels.empty())
		return;
	std::vector<Channel*> tempChannels;
	std::map<std::string, Channel*>::iterator it;
	for (it = channels.begin(); it != channels.end(); ++it)
		tempChannels.push_back(it->second);
	std::vector<Channel*>::iterator it1; 
	for (it1 = tempChannels.begin(); it1 != tempChannels.end(); ++it1)
	{
		if (*it1)
			delete *it1;
	}
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
	if (!cli || message.empty())
		return;
	std::string& buffer = cli->getSendBuffer();
	buffer.append(message);
	if (buffer.size() < 2 || buffer[buffer.size() - 2] != '\r' || buffer[buffer.size() - 1] != '\n')
		buffer.append("\r\n");
	for (size_t i = 0; i < pollfds.size(); ++i)
	{
		if (pollfds[i].fd == fd)
		{
			// If POLLOUT is already set, we don't need to do anything
			pollfds[i].events |= POLLOUT;
			break;
		}
	}
}

void Server::sendWelcome(Client *cli)
{
	if(!cli)
		return;
	int fd = cli->getSocketFd();
	std::string nick = cli->getNickname();
	sendResponse(fd, ":ircserv 001 " + nick + " :Welcome to the IRC Network, " + nick + "\r\n");
	sendResponse(fd, ":ircserv 002 " + nick + " :Your host is ircserv, running ft_irc" + "\r\n");
	sendResponse(fd, ":ircserv 003 " + nick + " :This server was created " + getDateCreated()+ "\r\n");
	sendResponse(fd, ":ircserv 004 " + nick + " ircserv 1.0 itkol"+ "\r\n");
	sendResponse(fd, ":ircserv 005 " + nick + " CHANTYPES=# PREFIX=(o)@ CHANMODES=i,t,k,l :are supported by this server\r\n");
}

void Server::cleanupEmptyChannels()
{
	std::vector<std::string> toErase;
	std::map<std::string, Channel*>::iterator it;
	for (it = channels.begin(); it != channels.end(); ++it)
	{
		Channel *ch = it->second;
		if (ch && ch->getChannelTotalClientCount() == 0)
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