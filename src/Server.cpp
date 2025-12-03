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
	for (size_t i = 0; i < pollfds.size(); ++i)
		close(pollfds[i].fd);
}

void Server::acceptClient()
{
	struct sockaddr_in clientAddr;
	socklen_t addrlen = sizeof(clientAddr);

	int clientFd = accept(socketFd, (struct sockaddr*)&clientAddr, &addrlen);
	if (clientFd < 0)
	{
		if (errno != EWOULDBLOCK)
			perror("accept");
		return;
	}

	std::cout << "Accepted new client: fd=" << clientFd << std::endl;

	struct pollfd p;
	p.fd = clientFd;
	p.events = POLLIN;
	p.revents = 0;
	pollfds.push_back(p);
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
