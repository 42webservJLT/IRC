//
// Created by Leon David Zipp on 1/7/25.
//

#include "Server.hpp"

Server::Server(uint16_t port, std::string password) : _port(port), _password(password) {
	//	open socket
	_socket = socket(AF_INET, SOCK_STREAM, 0);
	if (_socket == -1) {
		throw std::runtime_error("Failed to create socket");
	}

//	set socket options
	int flags = fcntl(_socket, F_GETFL, 0);
	fcntl(_socket, F_SETFL, flags | O_NONBLOCK);

	struct addrinfo hints;
	std::memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;

//	resolve hostname to IP address
	struct addrinfo* res;
	int status = getaddrinfo(server.GetConfig().GetHost().c_str(), std::to_string(server.GetConfig().GetPort()).c_str(), &hints, &res);
	if (status != 0) {
		std::cerr << "Error: Failed to resolve hostname: " << gai_strerror(status) << std::endl;
		close(_socket);
		throw std::runtime_error("Failed to resolve hostname");
	}

//	bind socket
	if (bind(_socket, res->ai_addr, res->ai_addrlen) == -1) {
		std::cerr << "Error: Failed to bind socket" << std::endl;
		freeaddrinfo(res);
		close(_socket);
		throw std::runtime_error("Failed to bind socket");
	}

	freeaddrinfo(res);

	//	initialize pollfd vector
	_pollFds.push_back({ _socket, POLLIN, 0 });
}

Server::~Server() {}

void Server::run() {
//	start listening
	if (listen(_socket, SOMAXCONN) < 0) {
		std::cerr << "Listen failed" << std::endl;
		close(_socket);
		throw std::runtime_error("Failed to listen");
	}

	std::cout << "Server running on port " << _port << " with password " << _password << std::endl;
}
