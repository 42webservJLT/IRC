//
// Created by Leon David Zipp on 1/7/25.
//

#include "Server.hpp"
#include "Client.hpp"

Mode _strToModeEnum(std::string str);

/* --------------------------------------------------------------------------------- */
/* Constructors & Destructors                                                        */
/* --------------------------------------------------------------------------------- */
Server::Server(uint16_t port, std::string password) : _host("127.0.0.1"), _port(port), _password(password) {
	//	open socket
	_socket = socket(AF_INET, SOCK_STREAM, 0);
	if (_socket == -1) {
		throw std::runtime_error("Failed to create socket");
	}

//	set socket options
	fcntl(_socket, F_SETFL, O_NONBLOCK);

	struct addrinfo hints;
	std::memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;

//	resolve hostname to IP address
	struct addrinfo *res;
	int status = getaddrinfo(GetHost().c_str(), std::to_string(GetPort()).c_str(), &hints, &res);
	if (status != 0) {
		close(_socket);
		throw std::runtime_error("Failed to resolve hostname");
	}

//	bind socket
	if (bind(_socket, res->ai_addr, res->ai_addrlen) == -1) {
		freeaddrinfo(res);
		close(_socket);
		throw std::runtime_error("Failed to bind socket");
	}

	freeaddrinfo(res);

	// IMPORTANT: listen on your bound socket
	if (listen(_socket, SOMAXCONN) < 0) {
		close(_socket);
		throw std::runtime_error("Failed to listen on socket");
	}

	// Make _listeningFd point to the same socket
	_listeningFd = _socket;

	//	initialize pollfd vector
	_pollFds.push_back({_socket, POLLIN, 0});

	// Print server start message
	std::cout << "Server running on " << _host << ":" << _port << std::endl;

//	initialize function mapping
	_methods.emplace(AUTHENTICATE, static_cast<void (Server::*)(int, const std::vector<std::string>&)>(&Server::Authenticate));
	_methods.emplace(NICK,         static_cast<void (Server::*)(int, const std::vector<std::string>&)>(&Server::Nick));
	_methods.emplace(USER,         static_cast<void (Server::*)(int, const std::vector<std::string>&)>(&Server::User));
	_methods.emplace(JOIN,         static_cast<void (Server::*)(int, const std::vector<std::string>&)>(&Server::Join));
	_methods.emplace(MSG,          static_cast<void (Server::*)(int, const std::vector<std::string>&)>(&Server::PrivMsg));
	_methods.emplace(KICK,         static_cast<void (Server::*)(int, const std::vector<std::string>&)>(&Server::Kick));
	_methods.emplace(INVITE,       static_cast<void (Server::*)(int, const std::vector<std::string>&)>(&Server::Invite));
	_methods.emplace(TOPIC,        static_cast<void (Server::*)(int, const std::vector<std::string>&)>(&Server::Topic));
	_methods.emplace(MODE,         static_cast<void (Server::*)(int, const std::vector<std::string>&)>(&Server::Mode));
	_methods.emplace(PING,         static_cast<void (Server::*)(int, const std::vector<std::string>&)>(&Server::Ping));
	_methods.emplace(QUIT,         static_cast<void (Server::*)(int, const std::vector<std::string>&)>(&Server::Quit));
	_methods.emplace(INVALID,      nullptr);

//	initialize parser
	_parser = Parser();
}

Server::~Server() {}

/* --------------------------------------------------------------------------------- */
/* Getters & Setters                                                                 */
/* --------------------------------------------------------------------------------- */
// returns the host
std::string Server::GetHost() const {
	return _host;
}

// returns the port number
uint16_t Server::GetPort() const {
	return _port;
}

// returns the password
std::string Server::GetPassword() const {
	return _password;
}

// returns the socket
int Server::GetSocket() const {
	return _socket;
}

// returns the pollfd vector
std::vector<pollfd> Server::GetPollFds() const {
	return _pollFds;
}

/* --------------------------------------------------------------------------------- */
/* Run                                                                               */
/* --------------------------------------------------------------------------------- */
// runs the server
bool Server::Run() {
	while (true) {
		int pollCount = poll(_pollFds.data(), _pollFds.size(), -1);
		if (pollCount < 0) {
			// handle error
			return false;
		}

		std::vector<int> toRemove;
		for (size_t i = 0; i < _pollFds.size(); ++i) {
			if (_pollFds[i].revents & POLLIN) {
				if (_pollFds[i].fd == _listeningFd) {
					HandleNewConnection();
				} else {
					if (!HandleClient(_pollFds[i].fd)) {
						toRemove.push_back(_pollFds[i].fd);
					}
				}
			}
			// optionally handle other events like POLLOUT
		}

		// Remove closed/disconnected FDs here
		for (size_t i = 0; i < toRemove.size(); ++i) {
			RemoveClient(toRemove[i]);
		}
	}
	return true;
}
