#ifndef SERVER_HPP
#define SERVER_HPP

#include <csignal> //signal()
#include <cstring> //memset(), strlen()
#include <cstdlib> //atoi(), exit()
#include <ctime> //time(), localtime()
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

#endif